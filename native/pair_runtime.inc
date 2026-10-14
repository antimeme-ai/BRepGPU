// Owned additive pair service, included once by runtime.cpp.
namespace {
static_assert(sizeof(bg_pair_box)==48 && sizeof(bg_pair_options)==24);
static_assert(sizeof(bg_pair_index_info)==24 && sizeof(bg_pair_page_info)==80);
static_assert(sizeof(bg_pair_abi_info)==40 && sizeof(bg_backend_v4)==72);
struct PairIndex final:Object {
    std::vector<bg_pair_box> boxes;
    void* token=nullptr;
    PairIndex():Object(Kind::PairIndex){}
    ~PairIndex(){if(token)cleanup_child(context,token,context->plugin->pairs->index_close);}
};
struct PairPage final:Object {
    std::shared_ptr<PairIndex> left,right;
    uint64_t start=0,count=0,total=0;
    std::vector<uint32_t> words;
    bg_backend_profile profile{0,0,std::numeric_limits<double>::quiet_NaN()};
    void* token=nullptr;
    PairPage():Object(Kind::PairPage){}
    ~PairPage(){if(token)cleanup_child(context,token,context->plugin->pairs->page_close);}
    uint64_t word_count()const noexcept{return count/32+(count%32!=0);}
};
template<class T>void pair_version(const T* p,uint64_t bytes){
    pointer(p,1);if(bytes<sizeof(T)||p->abi_version!=BG_PAIR_ABI_VERSION||p->struct_size!=sizeof(T))
        fail(BG_INVALID_ARGUMENT,"pair descriptor ABI or size mismatch");
}
const bg_backend_v4* pair_backend(const std::shared_ptr<Context>& context){
    auto&p=*context->plugin;
    if(p.pair_state==1)return p.pairs;
    if(p.pair_state==2)fail(BG_UNSUPPORTED,"CUDA plugin does not support pairs",BG_STAGE_PLUGIN);
    auto factory=reinterpret_cast<bg_pair_backend_factory_fn>(dlsym(p.library,BG_PAIR_BACKEND_FACTORY_SYMBOL));
    if(!factory){p.pair_state=2;fail(BG_UNSUPPORTED,"CUDA plugin has no pair extension",BG_STAGE_PLUGIN);}
    auto d=diagnostic();const bg_backend_v4* table=nullptr;
    const auto status=factory(BG_PAIR_BACKEND_ABI_VERSION,&table,&d);
    if(status==BG_UNSUPPORTED && !(d.flags&BG_DIAG_FATAL)){
        p.pair_state=2;fail(BG_UNSUPPORTED,"CUDA plugin refused pair extension",BG_STAGE_PLUGIN);
    }
    backend_status(status,d);
    if(!table||table->abi_version!=4||table->struct_size!=sizeof(bg_backend_v4)||table->base!=p.table
       ||!(table->capabilities&BG_BACKEND_CAP_PAIRS)||!table->index_upload||!table->index_close
       ||!table->index_download||!table->discover||!table->page_close||!table->page_download){
        p.pair_state=2;fail(BG_UNSUPPORTED,"CUDA pair extension layout/capability/base mismatch",BG_STAGE_PLUGIN);
    }
    p.pairs=table;p.pair_state=1;return table;
}
std::shared_ptr<PairIndex> pair_right(bg_handle id,const std::shared_ptr<Context>& context){
    // The first admission holds this context lock. Do not acquire a foreign one.
    auto object=lookup(id);
    if(object->kind!=Kind::PairIndex||object->context.get()!=context.get())
        fail(BG_INVALID_HANDLE,"wrong or foreign pair index");
    return std::static_pointer_cast<PairIndex>(object);
}
}
extern "C" bg_status bg4_pair_abi_info(bg_pair_abi_info*out,uint64_t bytes){
    return call(nullptr,0,[&](bg_diagnostic&){pair_version(out,bytes);
        *out={4,sizeof(*out),sizeof(bg_pair_box),sizeof(bg_pair_options),sizeof(bg_pair_index_info),
              sizeof(bg_pair_page_info),sizeof(bg_diagnostic),8,4,sizeof(bg_handle)};});
}
extern "C" bg_status bg4_create_pair_index(bg_handle id,const bg_pair_box*boxes,uint64_t count,
    uint64_t bytes,bg_handle*out,bg_diagnostic*diag,uint64_t db){
    if(discovery_alias({{out,1,sizeof(bg_handle)}},{{boxes,count,sizeof(bg_pair_box)}},diag))return BG_INVALID_ARGUMENT;
    zero_handle(out);return call(diag,db,[&](bg_diagnostic&){
        pointer(out,1);pointer(boxes,count);
        if(count>BG_PAIR_MAX_INDEX_COUNT||bytes!=multiply(count,sizeof(bg_pair_box)))fail(BG_INVALID_ARGUMENT,"pair index extent/cap mismatch");
        Admission a(id,Kind::Context);
        auto p=std::make_shared<PairIndex>();p->context=a.context;p->boxes=snapshot(boxes,count);
        for(const auto&box:p->boxes)if(!brepgpu::pair_detail::valid(box))fail(BG_INVALID_ARGUMENT,"nonfinite or unordered pair box");
        if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();const auto* backend=pair_backend(a.context);
            backend_status(backend->index_upload(a.context->token,p->boxes.data(),count,&p->token,&d),d);
            if(!p->token)fail(BG_INTERNAL_ERROR,"CUDA backend published no pair index");
            ++a.context->device_children;}
        *out=publish_owned(p,a.context);
    });
}
extern "C" bg_status bg4_pair_index_info(bg_handle id,bg_pair_index_info*out,uint64_t bytes,bg_diagnostic*diag,uint64_t db){
    if(discovery_alias({{out,1,sizeof(*out)}},{},diag))return BG_INVALID_ARGUMENT;
    return call(diag,db,[&](bg_diagnostic&){pair_version(out,bytes);Admission a(id,Kind::PairIndex);
        auto p=std::static_pointer_cast<PairIndex>(a.object);final_cuda_check(a.context);
        *out={4,sizeof(*out),p->boxes.size(),a.context->backend,0};});
}
extern "C" bg_status bg4_pair_index_download(bg_handle id,bg_pair_box*boxes,uint64_t count,uint64_t bytes,bg_diagnostic*diag,uint64_t db){
    if(discovery_alias({{boxes,count,sizeof(bg_pair_box)}},{},diag))return BG_INVALID_ARGUMENT;
    return call(diag,db,[&](bg_diagnostic&){pointer(boxes,count);Admission a(id,Kind::PairIndex);
        auto p=std::static_pointer_cast<PairIndex>(a.object);
        if(count!=p->boxes.size()||bytes!=multiply(count,sizeof(bg_pair_box)))fail(BG_INVALID_ARGUMENT,"pair index download extent mismatch",BG_STAGE_DOWNLOAD);
        if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();backend_status(a.context->plugin->pairs->index_download(a.context->token,p->token,boxes,count,&d),d);}
        else if(count)std::copy(p->boxes.begin(),p->boxes.end(),boxes);
        final_cuda_check(a.context);
    });
}
extern "C" bg_status bg4_discover_pairs(bg_handle left,bg_handle right,const bg_pair_options*options,uint64_t bytes,
    bg_handle*out,bg_diagnostic*diag,uint64_t db){
    if(discovery_alias({{out,1,sizeof(bg_handle)}},{{options,1,sizeof(*options)}},diag))return BG_INVALID_ARGUMENT;
    zero_handle(out);return call(diag,db,[&](bg_diagnostic&){pointer(out,1);pair_version(options,bytes);const auto input=*options;
        if(!input.max_pairs||input.max_pairs>BG_PAIR_MAX_PAIRS||input.flags)fail(BG_INVALID_ARGUMENT,"invalid pair page options");
        Admission a(left,Kind::PairIndex);auto r=std::make_shared<PairPage>();r->context=a.context;
        r->left=std::static_pointer_cast<PairIndex>(a.object);r->right=pair_right(right,a.context);
        r->total=multiply(r->left->boxes.size(),r->right->boxes.size());
        if(input.start>r->total)fail(BG_INVALID_ARGUMENT,"pair start exceeds total");
        r->start=input.start;r->count=std::min<uint64_t>(input.max_pairs,r->total-r->start);
        if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();const auto* backend=pair_backend(a.context);
            backend_status(backend->discover(a.context->token,r->left->token,r->right->token,&input,&r->token,&r->profile,&d),d);
            if(!r->token)fail(BG_INTERNAL_ERROR,"CUDA backend published no pair page");
            ++a.context->device_children;
        }else{
            const uint64_t words=r->word_count();r->words.resize(words);
            const uint64_t workers=std::min<uint64_t>(a.context->threads,words);
            if(words){auto range=[&](uint64_t begin,uint64_t end){for(uint64_t w=begin;w<end;++w)
                r->words[w]=brepgpu::pair_detail::word(r->left->boxes.data(),r->right->boxes.data(),r->right->boxes.size(),r->start,r->count,w);};
                if(workers<=1)range(0,words);else{
                    std::vector<std::thread> threads;threads.reserve(workers);
                    auto boundary=[&](uint64_t i){return(words/workers)*i+std::min<uint64_t>(i,words%workers);};
                    try{for(uint64_t i=0;i<workers;++i)threads.emplace_back(range,boundary(i),boundary(i+1));}
                    catch(...){for(auto&t:threads)if(t.joinable())t.join();throw;}
                    for(auto&t:threads)t.join();
                }
            }
        }
        *out=publish_owned(r,a.context);
    });
}
extern "C" bg_status bg4_pair_page_info(bg_handle id,bg_pair_page_info*out,uint64_t bytes,bg_diagnostic*diag,uint64_t db){
    if(discovery_alias({{out,1,sizeof(*out)}},{},diag))return BG_INVALID_ARGUMENT;
    return call(diag,db,[&](bg_diagnostic&){pair_version(out,bytes);Admission a(id,Kind::PairPage);
        auto r=std::static_pointer_cast<PairPage>(a.object);final_cuda_check(a.context);
        const uint64_t next=r->start+r->count;
        uint32_t flags=(next==r->total?BG_PAIR_REACHED_END:0u)|(!r->start&&r->count==r->total?BG_PAIR_FULL_DOMAIN:0u);
        double ms=std::numeric_limits<double>::quiet_NaN();
        if(a.context->backend==BG_BACKEND_CUDA&&r->count&&(r->profile.flags&BG_RESULT_PROFILED)){flags|=BG_PAIR_PROFILED;ms=r->profile.kernel_ms;}
        *out={4,sizeof(*out),r->total,r->start,r->count,next,r->left->boxes.size(),r->right->boxes.size(),r->word_count(),a.context->backend,flags,ms};
    });
}
extern "C" bg_status bg4_pair_page_download(bg_handle id,uint32_t*words,uint64_t count,uint64_t bytes,bg_diagnostic*diag,uint64_t db){
    if(discovery_alias({{words,count,4}},{},diag))return BG_INVALID_ARGUMENT;
    return call(diag,db,[&](bg_diagnostic&){pointer(words,count);Admission a(id,Kind::PairPage);auto r=std::static_pointer_cast<PairPage>(a.object);
        if(count!=r->word_count()||bytes!=multiply(count,4))fail(BG_INVALID_ARGUMENT,"pair page download extent mismatch",BG_STAGE_DOWNLOAD);
        if(a.context->backend==BG_BACKEND_CUDA){auto d=diagnostic();backend_status(a.context->plugin->pairs->page_download(a.context->token,r->token,words,count,&d),d);}
        else if(count)std::copy(r->words.begin(),r->words.end(),words);
        final_cuda_check(a.context);
    });
}
