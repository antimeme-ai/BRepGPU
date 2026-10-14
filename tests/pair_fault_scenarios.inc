bg_pair_options pair_options(){return {4,sizeof(bg_pair_options),0,1048576,0};}
bg_handle pair_index(Fixture&f){const bg_pair_box box{{0,0,0},{0,0,0}};auto d=diagnostic();bg_handle out=0;
    check(bg4_create_pair_index(f.context,&box,1,sizeof(box),&out,&d,sizeof(d))==BG_OK,"pair index setup failed");return out;}
bg_handle pair_page(bg_handle left,bg_handle right){auto o=pair_options();auto d=diagnostic();bg_handle out=0;
    check(bg4_discover_pairs(left,right,&o,sizeof(o),&out,&d,sizeof(d))==BG_OK,"pair page setup failed");return out;}
void check_pair_poison(Fixture&f,bg_handle index){
    auto d=diagnostic();auto o=pair_options();bg_handle out=99;
    check(bg4_discover_pairs(index,index,&o,sizeof(o),&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"prior fatal did not poison pairs");
    d=diagnostic();check(bg1_evaluate(f.query,0,&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"pair fatal did not poison evaluator");
}
bool pair_scenario(const std::string&scenario,const char*plugin){
    if(scenario.rfind("pair-",0)!=0&&scenario!="evaluation-poison-pairs")return false;
    if(scenario=="pair-parent-close"){
        Fixture f(plugin);const auto l=pair_index(f),r=pair_index(f),p=pair_page(l,r);
        close_ok(l);close_ok(r);check(bg_fault_live(9)==2&&bg_fault_live(10)==1,"pair retained indexes drained early");
        bg_pair_page_info info{4,sizeof(bg_pair_page_info)};auto d=diagnostic();
        check(bg4_pair_page_info(p,&info,sizeof(info),&d,sizeof(d))==BG_OK&&info.count==1&&info.flags==3,"retained pair info lost");
        uint32_t word=99;d=diagnostic();check(bg4_pair_page_download(p,&word,1,4,&d,sizeof(d))==BG_OK&&word==1,"retained pair mask lost");
        close_ok(p);check(bg_fault_live(9)==0&&bg_fault_live(10)==0,"pair retained graph leaked");close_ok(f.context);assert_drained();
    }else if(scenario=="pair-admission-close"){
        Fixture f(plugin);const auto l=pair_index(f);Gate gate;gate.target=l;bg_test_set_hook(pause_admission,&gate);bg_status status=99;bg_handle out=99;
        std::thread worker([&]{auto d=diagnostic();auto o=pair_options();status=bg4_discover_pairs(l,l,&o,sizeof(o),&out,&d,sizeof(d));});
        {std::unique_lock<std::mutex> lock(gate.mutex);check(gate.cv.wait_for(lock,std::chrono::seconds(10),[&]{return gate.arrived;}),"pair admission hook timed out");}
        close_ok(f.context);{std::lock_guard<std::mutex> lock(gate.mutex);gate.resume=true;}gate.cv.notify_all();worker.join();bg_test_set_hook(nullptr,nullptr);
        check(status==BG_INVALID_HANDLE&&out==0,"closed context admitted pair page");assert_drained();
    }else if(scenario=="pair-index-publication-oom-fatal-cleanup"||scenario=="pair-page-publication-oom-fatal-cleanup"){
        Fixture f(plugin);const bool index=scenario=="pair-index-publication-oom-fatal-cleanup";
        bg_handle l=0;if(!index)l=pair_index(f);
        bg_fault_mode(index?72:73);bg_test_set_hook(throw_publication,nullptr);bg_handle out=99;auto d=diagnostic();bg_status code;
        if(index){const bg_pair_box box{{0,0,0},{0,0,0}};code=bg4_create_pair_index(f.context,&box,1,sizeof(box),&out,&d,sizeof(d));}
        else{auto o=pair_options();code=bg4_discover_pairs(l,l,&o,sizeof(o),&out,&d,sizeof(d));}
        check(code==BG_OUT_OF_MEMORY&&out==0&&(d.flags&BG_DIAG_FATAL)&&std::strstr(d.message,"cleanup"),"pair publication lost OOM/fatal cleanup");
        bg_test_set_hook(nullptr,nullptr);bg_fault_mode(0);check(bg_fault_live(index?9:10)==0,"unpublished pair token leaked");
        d=diagnostic();check(bg1_evaluate(f.query,0,&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"pair publication poison did not cross evaluator");
        d=diagnostic();check(bg1_close(f.context,&d,sizeof(d))==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"pair cleanup diagnostic history lost");assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="pair-mask-download-fatal"||scenario=="pair-index-download-fatal"){
        Fixture f(plugin);const auto l=pair_index(f),p=pair_page(l,l);const auto pq=projection_query(f);const auto h=hierarchy(f),dq=discovery_query(h);
        auto d=diagnostic();bg_status code;bg_fault_mode(scenario=="pair-mask-download-fatal"?74:75);
        if(scenario=="pair-mask-download-fatal"){uint32_t w=99;code=bg4_pair_page_download(p,&w,1,4,&d,sizeof(d));check(w==123,"partial pair mask witness missing");}
        else{bg_pair_box b{};code=bg4_pair_index_download(l,&b,1,sizeof(b),&d,sizeof(d));check(b.lower[0]==123,"partial pair box witness missing");}
        check(code==BG_DEVICE_FAILURE&&(d.flags&BG_DIAG_FATAL),"partial pair copy reported success");bg_fault_mode(0);check_pair_poison(f,l);
        bg_handle out=99;auto po=options();d=diagnostic();check(bg2_project(pq,&po,sizeof(po),&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"pair fatal did not poison projection");
        auto o=discovery_options();d=diagnostic();check(bg3_discover(dq,&o,sizeof(o),&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"pair fatal did not poison discovery");
        close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="evaluation-poison-pairs"){
        Fixture f(plugin);const auto l=pair_index(f),p=pair_page(l,l);f.evaluate();uint32_t saved=99;auto d=diagnostic();
        check(bg4_pair_page_download(p,&saved,1,4,&d,sizeof(d))==BG_OK&&saved==1,"prior pair host copy failed");
        bg_fault_mode(2);double jets[3]{};uint32_t status=99;d=diagnostic();check(bg1_download(f.result,jets,3,&status,1,&d,sizeof(d))==BG_DEVICE_FAILURE,"prior-family fatal model failed");
        bg_fault_mode(0);check_pair_poison(f,l);check(saved==1,"completed pair host value changed after poison");close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="pair-upload-failure"||scenario=="pair-restore-failure"){
        Fixture f(plugin);bg_handle l=0,out=99;auto d=diagnostic();bg_status code;
        if(scenario=="pair-upload-failure"){bg_fault_mode(70);const bg_pair_box b{{0,0,0},{0,0,0}};code=bg4_create_pair_index(f.context,&b,1,sizeof(b),&out,&d,sizeof(d));}
        else{l=pair_index(f);bg_fault_mode(71);auto o=pair_options();code=bg4_discover_pairs(l,l,&o,sizeof(o),&out,&d,sizeof(d));}
        check(code==(scenario=="pair-upload-failure"?BG_OUT_OF_MEMORY:BG_DEVICE_FAILURE)&&out==0&&!(d.flags&BG_DIAG_FATAL),"pair callback failure publication mismatch");
        check(bg_fault_live(10)==0,"failed pair callback leaked page");bg_fault_mode(0);if(!l)l=pair_index(f);const auto p=pair_page(l,l);close_ok(p);close_ok(f.context);assert_drained();
    }else if(scenario=="pair-extension-refusal"||scenario=="pair-extension-base"||scenario=="pair-extension-size"){
        Fixture f(plugin);bg_fault_mode(scenario=="pair-extension-refusal"?80:scenario=="pair-extension-base"?81:82);const bg_pair_box b{{0,0,0},{0,0,0}};bg_handle out=99;auto d=diagnostic();
        check(bg4_create_pair_index(f.context,&b,1,sizeof(b),&out,&d,sizeof(d))==BG_UNSUPPORTED&&out==0,"bad pair extension admitted");bg_fault_mode(0);d=diagnostic();
        check(bg4_create_pair_index(f.context,&b,1,sizeof(b),&out,&d,sizeof(d))==BG_UNSUPPORTED&&out==0,"stable pair refusal not cached");check(bg_fault_factory_calls()==1,"pair refusal renegotiated");
        f.evaluate();const auto pq=projection_query(f),pr=projection_result(pq);close_ok(pr);const auto h=hierarchy(f);close_ok(h);close_ok(f.context);assert_drained();
    }else if(scenario=="pair-extension-transient"){
        Fixture f(plugin);bg_fault_mode(83);const bg_pair_box b{{0,0,0},{0,0,0}};bg_handle out=99;auto d=diagnostic();
        check(bg4_create_pair_index(f.context,&b,1,sizeof(b),&out,&d,sizeof(d))==BG_OUT_OF_MEMORY&&out==0,"pair transient factory mislabeled");bg_fault_mode(0);pair_index(f);
        check(bg_fault_factory_calls()==2,"pair transient factory failure cached");close_ok(f.context);assert_drained();
    }else if(scenario=="pair-extension-fatal"||scenario=="pair-extension-fatal-ok"||scenario=="pair-extension-fatal-unsupported"){
        Fixture f(plugin);bg_fault_mode(scenario=="pair-extension-fatal"?84:scenario=="pair-extension-fatal-ok"?85:86);
        const bg_pair_box b{{0,0,0},{0,0,0}};bg_handle out=99;auto d=diagnostic();const auto code=bg4_create_pair_index(f.context,&b,1,sizeof(b),&out,&d,sizeof(d));
        check(code==(scenario=="pair-extension-fatal-unsupported"?BG_UNSUPPORTED:BG_DEVICE_FAILURE)&&out==0&&(d.flags&BG_DIAG_FATAL),"pair factory fatal swallowed");
        bg_fault_mode(0);d=diagnostic();check(bg1_evaluate(f.query,0,&out,&d,sizeof(d))==BG_CONTEXT_UNUSABLE&&out==0,"pair factory fatal did not poison old evaluator");close_ok(f.context);assert_drained();cpu_still_operates(plugin);
    }else if(scenario=="pair-environment"){
        Fixture f(plugin,false,24);const unsigned original=_mm_getcsr();
        uint64_t bits=1;double tiny;std::memcpy(&tiny,&bits,8);
        const bg_pair_box zero{{0,0,0},{0,0,0}},reversed{{tiny,0,0},{0,0,0}};
        bg_pair_box positive[33];for(auto&b:positive)b={{tiny,0,0},{tiny,0,0}};
        const unsigned environments[]={original|0x8040u,(original&~0x6000u)|0x2000u,(original&~0x6000u)|0x6000u};
        for(unsigned environment:environments){
            _mm_setcsr(environment);auto d=diagnostic();bg_handle l=0,r=0,out=99;
            const auto bad=bg4_create_pair_index(f.context,&reversed,1,48,&out,&d,sizeof(d));
            const bool rejected=bad==BG_INVALID_ARGUMENT&&out==0;
            d=diagnostic();const auto left=bg4_create_pair_index(f.context,&zero,1,48,&l,&d,sizeof(d));
            d=diagnostic();const auto right=bg4_create_pair_index(f.context,positive,33,sizeof(positive),&r,&d,sizeof(d));
            auto o=pair_options();bg_handle page=0;d=diagnostic();const auto work=bg4_discover_pairs(l,r,&o,sizeof(o),&page,&d,sizeof(d));
            uint32_t words[2]={99,99};d=diagnostic();const auto copy=work==BG_OK?bg4_pair_page_download(page,words,2,8,&d,sizeof(d)):BG_INTERNAL_ERROR;
            _mm_setcsr(original);
            check(rejected&&left==BG_OK&&right==BG_OK&&work==BG_OK&&copy==BG_OK&&words[0]==0&&words[1]==0,"pair bit predicate depended on inherited FP environment");
            close_ok(page);close_ok(l);close_ok(r);
        }
        close_ok(f.context);assert_drained();
    }else if(scenario=="pair-alias-extents"){
        Fixture f(plugin);const auto l=pair_index(f),p=pair_page(l,l);auto d=diagnostic();auto o=pair_options();
        auto*out=new(d.message) bg_handle(99);auto saved=d;
        check(bg4_discover_pairs(l,l,&o,sizeof(o),out,&d,sizeof(d))==BG_INVALID_ARGUMENT&&std::memcmp(&d,&saved,sizeof(d))==0,"pair creator diagnostic alias wrote");
        auto*info=new(d.message) bg_pair_page_info{};info->abi_version=4;info->struct_size=sizeof(*info);saved=d;
        check(bg4_pair_page_info(p,info,sizeof(*info),&d,sizeof(d))==BG_INVALID_ARGUMENT&&std::memcmp(&d,&saved,sizeof(d))==0,"pair info diagnostic alias wrote");
        d=diagnostic();saved=d;check(bg4_pair_page_download(p,reinterpret_cast<uint32_t*>(&d),1,4,&d,sizeof(d))==BG_INVALID_ARGUMENT&&std::memcmp(&d,&saved,sizeof(d))==0,"pair mask diagnostic alias wrote");
        uint32_t word=99;d=diagnostic();check(bg4_pair_page_download(p,&word,1,8,&d,sizeof(d))==BG_INVALID_ARGUMENT&&word==99,"pair wrong bytes wrote output");
        d=diagnostic();saved=d;bg_handle handle=99;const auto*bad=reinterpret_cast<bg_pair_box*>(UINTPTR_MAX-16);
        check(bg4_create_pair_index(f.context,bad,1,48,&handle,&d,sizeof(d))==BG_INVALID_ARGUMENT&&handle==99&&std::memcmp(&d,&saved,sizeof(d))==0,"pair overflowing span preflight wrote");
        d=diagnostic();check(bg4_pair_page_download(p,&word,1,4,&d,sizeof(d))==BG_OK&&word==1,"pair rejection damaged result");close_ok(f.context);assert_drained();
    }else throw std::runtime_error("unknown pair scenario");
    return true;
}
