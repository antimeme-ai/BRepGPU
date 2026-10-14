// Included inside the existing CUDA backend's private namespace.
struct DevicePairIndex { Context* context=nullptr;bg_pair_box* boxes=nullptr;uint64_t count=0; };
struct DevicePairPage { Context* context=nullptr;uint32_t* words=nullptr;uint64_t word_count=0; };
void destroy(DevicePairIndex*p,Operation&op) noexcept {
    {DeviceScope device(p->context->device,BG_STAGE_CLOSE,op);if(device.selected){
        finish(p->context,op,BG_STAGE_CLOSE);
        if(p->boxes)op.cuda(cudaFree(p->boxes),BG_STAGE_CLOSE,"free pair boxes");
    }}delete p;
}
void destroy(DevicePairPage*p,Operation&op) noexcept {
    {DeviceScope device(p->context->device,BG_STAGE_CLOSE,op);if(device.selected){
        finish(p->context,op,BG_STAGE_CLOSE);
        if(p->words)op.cuda(cudaFree(p->words),BG_STAGE_CLOSE,"free pair words");
    }}delete p;
}
__global__ void pair_kernel(const bg_pair_box*left,const bg_pair_box*right,uint64_t right_count,
    uint64_t start,uint64_t count,uint64_t words,uint32_t*out){
    const uint64_t stride=uint64_t(blockDim.x)*gridDim.x;
    for(uint64_t w=uint64_t(blockIdx.x)*blockDim.x+threadIdx.x;w<words;w+=stride)
        out[w]=brepgpu::pair_detail::word(left,right,right_count,start,count,w);
}
bg_status pair_index_upload(void*ctx,const bg_pair_box*boxes,uint64_t count,void**out,bg_diagnostic*diag)noexcept{
    if(out)*out=nullptr;
    return guarded(BG_STAGE_UPLOAD,diag,[&](Operation&op){
        if(!ctx||!out||count>BG_PAIR_MAX_INDEX_COUNT||(count&&!boxes)){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_UPLOAD,"invalid pair index upload");return;}
        for(uint64_t i=0;i<count;++i)if(!brepgpu::pair_detail::valid(boxes[i])){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_UPLOAD,"nonfinite or unordered pair box");return;}
        auto*c=static_cast<Context*>(ctx);Pending<DevicePairIndex> pending(new DevicePairIndex{},op);
        auto*p=pending.value;p->context=c;p->count=count;
        {DeviceScope device(c->device,BG_STAGE_UPLOAD,op);if(device.selected&&op.ok()){
            allocate(p->boxes,count,c,op,BG_STAGE_UPLOAD);upload(p->boxes,boxes,count,c,op);finish(c,op,BG_STAGE_UPLOAD);
        }}if(op.ok())*out=pending.release();
    });
}
bg_status pair_index_close(void*ctx,void*token,bg_diagnostic*diag)noexcept{
    return guarded(BG_STAGE_CLOSE,diag,[&](Operation&op){
        if(!ctx||!token||static_cast<DevicePairIndex*>(token)->context!=ctx)
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_CLOSE,"invalid pair index close");
        else destroy(static_cast<DevicePairIndex*>(token),op);
    });
}
bg_status pair_index_download(void*ctx,void*token,bg_pair_box*boxes,uint64_t count,bg_diagnostic*diag)noexcept{
    return guarded(BG_STAGE_DOWNLOAD,diag,[&](Operation&op){
        if(!ctx||!token){op.error(BG_INVALID_ARGUMENT,BG_STAGE_DOWNLOAD,"missing pair index download input");return;}
        auto*c=static_cast<Context*>(ctx);auto*p=static_cast<DevicePairIndex*>(token);
        if(p->context!=c||count!=p->count||(count&&!boxes)){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_DOWNLOAD,"pair index download extent/context mismatch");return;}
        {DeviceScope device(c->device,BG_STAGE_DOWNLOAD,op);if(device.selected&&op.ok()){
            download_array(boxes,p->boxes,count,c,op);finish(c,op,BG_STAGE_DOWNLOAD);
        }}
    });
}
bg_status pair_discover(void*ctx,void*left,void*right,const bg_pair_options*options,
    void**out,bg_backend_profile*profile,bg_diagnostic*diag)noexcept{
    if(out)*out=nullptr;
    if(profile)*profile={0,0,std::numeric_limits<double>::quiet_NaN()};
    return guarded(BG_STAGE_EVALUATE,diag,[&](Operation&op){
        if(!ctx||!left||!right||!options||!out||!profile||options->abi_version!=4
            ||options->struct_size!=sizeof(*options)||!options->max_pairs||options->max_pairs>BG_PAIR_MAX_PAIRS||options->flags){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_EVALUATE,"invalid pair discovery input");return;}
        auto*c=static_cast<Context*>(ctx);auto*l=static_cast<DevicePairIndex*>(left);auto*r=static_cast<DevicePairIndex*>(right);
        if(l->context!=c||r->context!=c||l->count>BG_PAIR_MAX_INDEX_COUNT||r->count>BG_PAIR_MAX_INDEX_COUNT){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_EVALUATE,"pair context/count mismatch");return;}
        const uint64_t total=l->count*r->count;
        if(options->start>total){op.error(BG_INVALID_ARGUMENT,BG_STAGE_EVALUATE,"pair start exceeds total");return;}
        const uint64_t count=std::min<uint64_t>(options->max_pairs,total-options->start);
        const uint64_t words=count/32+(count%32!=0);
        Pending<DevicePairPage> pending(new DevicePairPage{},op);auto*p=pending.value;p->context=c;p->word_count=words;
        float milliseconds=0;bool measured=false;
        {DeviceScope device(c->device,BG_STAGE_EVALUATE,op);if(device.selected&&op.ok()){
            allocate(p->words,words,c,op,BG_STAGE_EVALUATE);
            cudaEvent_t start=nullptr,stop=nullptr;
            if(op.ok()&&c->profile&&words){op.cuda(cudaEventCreate(&start),BG_STAGE_EVALUATE,"create pair start event");
                if(op.ok())op.cuda(cudaEventCreate(&stop),BG_STAGE_EVALUATE,"create pair stop event");
                if(op.ok())op.cuda(cudaEventRecord(start,c->stream),BG_STAGE_EVALUATE,"record pair start");}
            if(op.ok()&&words){constexpr unsigned threads=128;
                const unsigned blocks=static_cast<unsigned>(std::min<uint64_t>(4096,words/threads+(words%threads!=0)));
                pair_kernel<<<blocks,threads,0,c->stream>>>(l->boxes,r->boxes,r->count,options->start,count,words,p->words);
                op.cuda(cudaGetLastError(),BG_STAGE_EVALUATE,"launch pair kernel");
                if(op.ok()&&stop)op.cuda(cudaEventRecord(stop,c->stream),BG_STAGE_EVALUATE,"record pair stop");}
            finish(c,op,BG_STAGE_EVALUATE);
            if(op.ok()&&start&&stop){op.cuda(cudaEventElapsedTime(&milliseconds,start,stop),BG_STAGE_EVALUATE,"read pair duration");measured=op.ok();}
            if(start)op.cuda(cudaEventDestroy(start),BG_STAGE_CLOSE,"destroy pair start event");
            if(stop)op.cuda(cudaEventDestroy(stop),BG_STAGE_CLOSE,"destroy pair stop event");
        }}if(op.ok()){if(measured)*profile={BG_RESULT_PROFILED,0,milliseconds};*out=pending.release();}
    });
}
bg_status pair_page_close(void*ctx,void*token,bg_diagnostic*diag)noexcept{
    return guarded(BG_STAGE_CLOSE,diag,[&](Operation&op){
        if(!ctx||!token||static_cast<DevicePairPage*>(token)->context!=ctx)
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_CLOSE,"invalid pair page close");
        else destroy(static_cast<DevicePairPage*>(token),op);
    });
}
bg_status pair_page_download(void*ctx,void*token,uint32_t*words,uint64_t count,bg_diagnostic*diag)noexcept{
    return guarded(BG_STAGE_DOWNLOAD,diag,[&](Operation&op){
        if(!ctx||!token){op.error(BG_INVALID_ARGUMENT,BG_STAGE_DOWNLOAD,"missing pair page download input");return;}
        auto*c=static_cast<Context*>(ctx);auto*p=static_cast<DevicePairPage*>(token);
        if(p->context!=c||count!=p->word_count||(count&&!words)){
            op.error(BG_INVALID_ARGUMENT,BG_STAGE_DOWNLOAD,"pair page download extent/context mismatch");return;}
        {DeviceScope device(c->device,BG_STAGE_DOWNLOAD,op);if(device.selected&&op.ok()){
            download_array(words,p->words,count,c,op);finish(c,op,BG_STAGE_DOWNLOAD);
        }}
    });
}
const bg_backend_v4 pair_table={4,sizeof(bg_backend_v4),BG_BACKEND_CAP_PAIRS,&table,
    pair_index_upload,pair_index_close,pair_index_download,pair_discover,pair_page_close,pair_page_download};
