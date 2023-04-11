#include <nan.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/resource.h> // setrlimit, getrlimit

using v8::Array;
using v8::FunctionTemplate;
using v8::Integer;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

struct name_to_int_t {
  const char* name;
  int resource;
};

static const name_to_int_t rlimit_name_to_res[] = {
  { "core", RLIMIT_CORE },
  { "cpu", RLIMIT_CPU },
  { "data", RLIMIT_DATA },
  { "fsize", RLIMIT_FSIZE },
  { "nofile", RLIMIT_NOFILE },
  #ifdef RLIMIT_NPROC
  { "nproc", RLIMIT_NPROC },
    #endif
  { "stack", RLIMIT_STACK },
  #ifdef RLIMIT_AS
  { "as", RLIMIT_AS },
  #endif
  { 0, 0 }
};

// return null if value is RLIM_INFINITY, otherwise the uint value
static Local<Value> rlimit_value(rlim_t limit) {
    if (limit == RLIM_INFINITY) {
        return Nan::Null();
    } else {
        return Nan::New<Number>((double)limit);
    }
}

NAN_METHOD(node_getrlimit) {
    Nan::HandleScope scope;

    if (info.Length() != 1) {
        return Nan::ThrowError("getrlimit: requires exactly one argument");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("getrlimit: argument must be a string");
    }

    struct rlimit limit;
    Nan::Utf8String rlimit_name(info[0]);
    int resource = -1;

    for (const name_to_int_t* item = rlimit_name_to_res; item->name; ++item) {
        if (!strcmp(*rlimit_name, item->name)) {
            resource = item->resource;
            break;
        }
    }

    if (resource < 0) {
        return Nan::ThrowError("getrlimit: unknown resource name");
    }

    if (getrlimit(resource, &limit)) {
        return Nan::ThrowError(Nan::ErrnoException(errno, "getrlimit", ""));
    }

    Local<Object> data = Nan::New<Object>();
    Nan::Set(data, Nan::New<String>("soft").ToLocalChecked(), rlimit_value(limit.rlim_cur));
    Nan::Set(data, Nan::New<String>("hard").ToLocalChecked(), rlimit_value(limit.rlim_max));

    info.GetReturnValue().Set(data);
}

NAN_METHOD(node_setrlimit) {
    Nan::HandleScope scope;

    if (info.Length() != 2) {
        return Nan::ThrowError("setrlimit: requires exactly two arguments");
    }

    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("setrlimit: argument 0 must be a string");
    }

    if (!info[1]->IsObject()) {
        return Nan::ThrowTypeError("setrlimit: argument 1 must be an object");
    }

    Nan::Utf8String rlimit_name(info[0]);
    int resource = -1;
    for (const name_to_int_t* item = rlimit_name_to_res; item->name; ++item) {
        if (!strcmp(*rlimit_name, item->name)) {
            resource = item->resource;
            break;
        }
    }

    if (resource < 0) {
        return Nan::ThrowError("setrlimit: unknown resource name");
    }

    Local<Object> limit_in = Nan::To<v8::Object>(info[1]).ToLocalChecked(); // Cast
    Local<String> soft_key = Nan::New<String>("soft").ToLocalChecked();
    Local<String> hard_key = Nan::New<String>("hard").ToLocalChecked();
    struct rlimit limit;
    bool get_soft = false, get_hard = false;
    if (Nan::Has(limit_in, soft_key).ToChecked()) {
        if (Nan::Get(limit_in, soft_key).ToLocalChecked()->IsNull()) {
            limit.rlim_cur = RLIM_INFINITY;
        } else {
            limit.rlim_cur = Nan::To<v8::Integer>(Nan::Get(limit_in, soft_key).ToLocalChecked()).ToLocalChecked()->Value();
        }
    } else {
        get_soft = true;
    }

    if (Nan::Has(limit_in, hard_key).ToChecked()) {
        if (Nan::Get(limit_in, hard_key).ToLocalChecked()->IsNull()) {
            limit.rlim_max = RLIM_INFINITY;
        } else {
            limit.rlim_max = Nan::To<v8::Integer>(Nan::Get(limit_in, hard_key).ToLocalChecked()).ToLocalChecked()->Value();
        }
    } else {
        get_hard = true;
    }

    if (get_soft || get_hard) {
        // current values for the limits are needed
        struct rlimit current;
        if (getrlimit(resource, &current)) {
            return Nan::ThrowError(Nan::ErrnoException(errno, "getrlimit", ""));
        }
        if (get_soft) { limit.rlim_cur = current.rlim_cur; }
        if (get_hard) { limit.rlim_max = current.rlim_max; }
    }

    if (setrlimit(resource, &limit)) {
        return Nan::ThrowError(Nan::ErrnoException(errno, "setrlimit", ""));
    }

    info.GetReturnValue().Set(Nan::Undefined());
}


#define EXPORT(name, symbol) Nan::Set(exports, \
  Nan::New<String>(name).ToLocalChecked(), \
  Nan::GetFunction(Nan::New<FunctionTemplate>(symbol)).ToLocalChecked()    \
)

void init(Local<Object> exports) {
    EXPORT("getrlimit", node_getrlimit);
    EXPORT("setrlimit", node_setrlimit);
}

NODE_MODULE(posix, init);
