#include "Rules/MaiRulesVM.h"
#include <chrono>
#include <cstring>
#include <quickjs.h>
namespace mai {
namespace { constexpr std::size_t Limit = 8 * 1024 * 1024; }
struct RulesVM::Impl {
    JSRuntime* runtime = nullptr;
    JSContext* context = nullptr;
    JSValue api = JS_UNDEFINED;
    std::chrono::steady_clock::time_point deadline;
    bool ready = false;
    static int Interrupt(JSRuntime*, void* opaque) {
        return std::chrono::steady_clock::now() > static_cast<Impl*>(opaque)->deadline;
    }
    bool Failure(std::string& error) {
        const JSValue exception = JS_GetException(context);
        const char* text = JS_ToCString(context, exception);
        error = text ? text : "Rules VM exception (out of memory or execution limit)";
        if (text) JS_FreeCString(context, text);
        JS_FreeValue(context, exception);
        return false;
    }
    bool Function(const char* name, int argc, JSValueConst* argv, JSValue& result, std::string& error) {
        const JSValue fn = JS_GetPropertyStr(context, api, name);
        if (!JS_IsFunction(context, fn)) { JS_FreeValue(context, fn); error = "Missing native rules entry point"; return false; }
        result = JS_Call(context, fn, api, argc, argv);
        JS_FreeValue(context, fn);
        if (JS_IsException(result)) return Failure(error);
        return true;
    }
};
RulesVM::RulesVM() : impl(new Impl) {}
RulesVM::~RulesVM() { Close(); }
bool RulesVM::IsReady() const { return impl->ready; }
void RulesVM::Close() {
    impl->ready = false;
    if (impl->context) { JS_FreeValue(impl->context, impl->api); impl->api=JS_UNDEFINED; JS_FreeContext(impl->context); impl->context=nullptr; }
    if (impl->runtime) { JS_FreeRuntime(impl->runtime); impl->runtime=nullptr; }
}
bool RulesVM::Open(const std::string& bundle, std::uint32_t seed, std::string& error) {
    Close(); error.clear();
    if (bundle.empty() || bundle.size()>Limit || seed==0) { error="Invalid trusted bundle or seed"; return false; }
    impl->runtime=JS_NewRuntime();
    if (!impl->runtime) { error="Cannot allocate rules runtime"; return false; }
    JS_SetMemoryLimit(impl->runtime,128*1024*1024);
    JS_SetMaxStackSize(impl->runtime,256*1024);
    impl->deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    JS_SetInterruptHandler(impl->runtime,Impl::Interrupt,impl.get());
    impl->context=JS_NewContext(impl->runtime);
    if (!impl->context) { error="Cannot allocate rules context"; Close(); return false; }
    const JSValue evaluated=JS_Eval(impl->context,bundle.c_str(),bundle.size(),"mai-rules.js",JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(evaluated)) { impl->Failure(error); Close(); return false; }
    JS_FreeValue(impl->context,evaluated);
    const JSValue global=JS_GetGlobalObject(impl->context);
    impl->api=JS_GetPropertyStr(impl->context,global,"MaiNative");
    JS_FreeValue(impl->context,global);
    if (!JS_IsObject(impl->api)) { error="Rules bundle has no MaiNative object"; Close(); return false; }
    impl->ready=true;
    std::string response;
    if (!Call("{\"method\":\"boot\",\"seed\":"+std::to_string(seed)+"}",response,error)) { Close(); return false; }
    // Boot entry reports business errors in JSON; initialization must be successful.
    if (response.find("\"ok\":true")!=0 && response.rfind("{\"ok\":true",0)!=0) { error=response; Close(); return false; }
    return true;
}
bool RulesVM::Call(const std::string& request, std::string& response, std::string& error) {
    response.clear(); error.clear();
    if (!impl->ready || request.size()>Limit) { error="Rules runtime unavailable or request too large"; return false; }
    impl->deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    JSValue argument=JS_NewStringLen(impl->context,request.data(),request.size()), result=JS_UNDEFINED;
    if (JS_IsException(argument)) return impl->Failure(error);
    const bool called=impl->Function("call",1,&argument,result,error);
    JS_FreeValue(impl->context,argument);
    if (!called) return false;
    JS_FreeValue(impl->context,result);
    for (unsigned jobs=0;jobs<10000;++jobs) {
        JSValue output=JS_UNDEFINED;
        if (!impl->Function("takeResponse",0,nullptr,output,error)) return false;
        if (JS_IsString(output)) {
            std::size_t length=0;
            const char* text=JS_ToCStringLen(impl->context,&length,output);
            if (!text) { JS_FreeValue(impl->context,output); return impl->Failure(error); }
            if (length<=Limit) response.assign(text,length);
            JS_FreeCString(impl->context,text); JS_FreeValue(impl->context,output);
            if (length>Limit) { error="Rules response too large"; return false; }
            return true;
        }
        JS_FreeValue(impl->context,output);
        JSContext* context=nullptr;
        const int status=JS_ExecutePendingJob(impl->runtime,&context);
        if (status<0) return impl->Failure(error);
        if (status==0) { error="Rules request did not produce a response"; return false; }
        if (std::chrono::steady_clock::now()>impl->deadline) { error="Rules request exceeded execution limit"; return false; }
    }
    error="Rules job limit reached"; return false;
}
}
