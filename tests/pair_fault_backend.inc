// Test-only lifetime/error model; not an independent numeric implementation.
struct PairModelIndex {std::vector<bg_pair_box> boxes;};
struct PairModelPage {uint64_t words;};
bg_status pair_upload(void*,const bg_pair_box*b,uint64_t n,void**out,bg_diagnostic*d){
    *out=nullptr;if(mode==70)return allocation_error(d,BG_STAGE_UPLOAD);
    PairModelIndex*p=nullptr;
    try{p=new PairModelIndex;if(n)p->boxes.assign(b,b+n);++pair_indices;*out=p;return BG_OK;}
    catch(const std::bad_alloc&){delete p;return allocation_error(d,BG_STAGE_UPLOAD);}
}
bg_status pair_index_close(void*,void*token,bg_diagnostic*d){delete static_cast<PairModelIndex*>(token);--pair_indices;
    if(mode==72)return error(d,"modeled fatal unpublished pair index cleanup",true,BG_STAGE_CLOSE);return BG_OK;}
bg_status pair_index_download(void*,void*token,bg_pair_box*b,uint64_t n,bg_diagnostic*d){
    if(mode==75){if(n)b[0].lower[0]=123;return error(d,"modeled fatal partial pair index copy",true,BG_STAGE_DOWNLOAD);}
    auto*p=static_cast<PairModelIndex*>(token);for(uint64_t i=0;i<n;++i)b[i]=p->boxes[i];return BG_OK;
}
bg_status pair_discover(void*,void*left,void*right,const bg_pair_options*o,void**out,bg_backend_profile*profile,bg_diagnostic*d){
    *out=nullptr;if(mode==71)return error(d,"modeled pair restoration failure",false,BG_STAGE_EVALUATE);
    const uint64_t total=static_cast<PairModelIndex*>(left)->boxes.size()*static_cast<PairModelIndex*>(right)->boxes.size();
    const uint64_t count=std::min<uint64_t>(o->max_pairs,total-o->start);
    try{*out=new PairModelPage{count/32+(count%32!=0)};++pair_pages;*profile={0,0,0};return BG_OK;}
    catch(const std::bad_alloc&){return allocation_error(d,BG_STAGE_EVALUATE);}
}
bg_status pair_page_close(void*,void*token,bg_diagnostic*d){delete static_cast<PairModelPage*>(token);--pair_pages;
    if(mode==73)return error(d,"modeled fatal unpublished pair page cleanup",true,BG_STAGE_CLOSE);return BG_OK;}
bg_status pair_page_download(void*,void*,uint32_t*w,uint64_t n,bg_diagnostic*d){
    if(mode==74){if(n)w[0]=123;return error(d,"modeled fatal partial pair mask copy",true,BG_STAGE_DOWNLOAD);}
    for(uint64_t i=0;i<n;++i)w[i]=1;return BG_OK;
}
const bg_backend_v4 pair_extension={4,sizeof(bg_backend_v4),BG_BACKEND_CAP_PAIRS,&table,
    pair_upload,pair_index_close,pair_index_download,pair_discover,pair_page_close,pair_page_download};
const bg_backend_v4 pair_wrong_base=[](){auto v=pair_extension;v.base=&other_base;return v;}();
const bg_backend_v4 pair_wrong_size=[](){auto v=pair_extension;v.struct_size-=8;return v;}();
