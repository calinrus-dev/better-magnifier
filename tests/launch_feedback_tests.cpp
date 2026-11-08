#define PINCH_ZOOM_STANDALONE
#define wmain standalone_main
#include "../mods/kde-pinch-zoom.wh.cpp"
#undef wmain
#include <iostream>
namespace {
struct Inputs final : shell_feedback::CreateInputs {
    DWORD startupFlags = 0; unsigned calls = 0; HRESULT outcome = S_OK;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void** out) override { if (out) *out=nullptr; return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override {return 1;} ULONG STDMETHODCALLTYPE Release() override{return 1;}
    HRESULT STDMETHODCALLTYPE GetCreateFlags(DWORD*) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetCreateFlags(DWORD) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE AddCreateFlags(DWORD) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetHotKey(WORD) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE AddStartupFlags(DWORD flags) override{++calls;startupFlags |= flags;return outcome;}
    HRESULT STDMETHODCALLTYPE SetTitle(LPCWSTR) override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE SetEnvironmentVariable(LPCWSTR,LPCWSTR) override{return E_NOTIMPL;}
};
Inputs captured;
DWORD receivedFlags = 0; bool receivedSite = false;
BOOL WINAPI fakeExecute(SHELLEXECUTEINFOW* launch) {
    receivedFlags=launch->fMask; receivedSite=(receivedFlags & SEE_MASK_FLAG_HINST_IS_SITE)!=0;
    if(receivedSite) {
        auto* provider=reinterpret_cast<IServiceProvider*>(launch->hInstApp);
        shell_feedback::CreatingProcess* creating=nullptr;
        if(FAILED(provider->QueryService(shell_feedback::kCreatingProcess,shell_feedback::kCreatingProcess,reinterpret_cast<void**>(&creating))))return FALSE;
        const HRESULT result=creating->OnCreating(&captured); creating->Release();
        if(FAILED(result))return FALSE;
    }
    launch->hProcess=reinterpret_cast<HANDLE>(0x1234);
    SetLastError(ERROR_ALREADY_EXISTS);
    return TRUE;
}
}
int wmain() {
    unsigned checks=0;
    const auto require=[&](bool valid,const char* message){if(!valid){std::cerr<<"FAIL "<<message<<'\n';std::exit(1);}++checks;};
    auto* site=new shell_feedback::Site;
    require(site->QueryInterface(shell_feedback::kUnknown,nullptr)==E_POINTER,"QI null output");
    void* canonical=nullptr; require(SUCCEEDED(site->QueryInterface(shell_feedback::kUnknown,&canonical))&&canonical==static_cast<IServiceProvider*>(site),"canonical unknown identity");
    static_cast<IUnknown*>(static_cast<IServiceProvider*>(canonical))->Release();
    shell_feedback::CreatingProcess* creating=nullptr;
    require(SUCCEEDED(site->QueryService(shell_feedback::kCreatingProcess,shell_feedback::kCreatingProcess,reinterpret_cast<void**>(&creating)))&&creating,"service resolves creating process");
    void* identity=nullptr; require(SUCCEEDED(creating->QueryInterface(shell_feedback::kUnknown,&identity))&&identity==static_cast<IServiceProvider*>(site),"creating interface shares canonical unknown");
    static_cast<IServiceProvider*>(identity)->Release();
    require(creating->OnCreating(nullptr)==E_POINTER,"OnCreating refuses null inputs");
    Inputs inputs; require(SUCCEEDED(creating->OnCreating(&inputs))&&inputs.calls==1&&inputs.startupFlags==STARTF_FORCEOFFFEEDBACK,"only the startup feedback flag is requested");
    inputs.outcome=E_ACCESSDENIED;require(creating->OnCreating(&inputs)==S_OK,"cosmetic startup hint failure cannot prevent launch");
    creating->Release();
    void* absent=reinterpret_cast<void*>(1);require(site->QueryService(shell_feedback::kUnknown,shell_feedback::kUnknown,&absent)==E_NOINTERFACE&&!absent,"unrelated services unchanged");
    require(site->Release()==0,"site owner releases final reference");
    SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.nShow=SW_HIDE;
    require(shell_feedback::execute(launch,false,fakeExecute)&&!receivedSite&&receivedFlags==SEE_MASK_NOCLOSEPROCESS,"opt out leaves launch unchanged");
    launch={};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_NOCLOSEPROCESS;launch.nShow=SW_HIDE;
    require(shell_feedback::execute(launch,true,fakeExecute)&&receivedSite&&captured.startupFlags==STARTF_FORCEOFFFEEDBACK,"enabled setting passes a public launch site");
    require((receivedFlags & SEE_MASK_NOASYNC)&&!(receivedFlags & SEE_MASK_WAITFORINPUTIDLE),"callback is synchronous without waiting for GUI idle");
    require(launch.hProcess==reinterpret_cast<HANDLE>(0x1234)&&launch.nShow==SW_HIDE,"process handle and show state preserved");
    require(GetLastError()==ERROR_ALREADY_EXISTS,"COM cleanup preserves the original launcher error status");
    std::cout<<checks<<" Shell feedback checks passed; fake launcher, no process created\n";
}
