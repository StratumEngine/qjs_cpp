#pragma once

#include "qjs/context_fwd.hpp"
#include "qjs/value_fwd.hpp"
#include "quickjs.h"

namespace Qjs {
    template <typename T>
    struct ClassDeleteTraits {
        static constexpr bool ShouldDelete = true;
    };

    template <typename T>
    struct ClassWrapper {
        private:
        inline static JSClassID classId = 0;
        static inline std::vector<Value T::*> markOffsets;

        public:
        static JSClassID GetClassId(Runtime &rt) {
            if (classId != 0)
                return classId;

            JS_NewClassID(rt, &classId);
            return classId;
        }

        static void RegisterClass(Context &ctx, std::string &&name, JSClassCall *invoker = nullptr) {
            if (JS_IsRegisteredClass(ctx.rt, GetClassId(ctx.rt)))
                return;

            JSClassGCMark *marker = nullptr;
            marker = [](JSRuntime *__rt, JSValue val, JS_MarkFunc *mark_func) {
                auto _rt = Runtime::From(__rt);
                if (!_rt)
                    return;
                auto &rt = *_rt;

                auto ptr = static_cast<T *>(JS_GetOpaque(val, GetClassId(rt)));
                if (!ptr)
                    return;

                for (Value T::*member : markOffsets)
                    JS_MarkValue(rt, (*ptr.*member).value, mark_func);
            };

            JSClassDef def{
                name.c_str(),
                [](JSRuntime *__rt, JSValue obj) noexcept {
                    if (!ClassDeleteTraits<T>::ShouldDelete)
                        return;

                    auto _rt = Runtime::From(__rt);
                    if (!_rt)
                        return;
                    auto &rt = *_rt;
                    
                    auto ptr = static_cast<T*>(JS_GetOpaque(obj, GetClassId(rt)));
                    delete ptr;
                },
                marker,
                invoker,
                nullptr
            };

            JS_NewClass(ctx.rt, GetClassId(ctx.rt), &def);
        }

        static void SetProto(Value proto);

        static Value New(Context &ctx, T *value) {
            Value val = Value::CreateFree(ctx, JS_NewObjectClass(ctx, GetClassId(ctx.rt)));
            JS_SetOpaque(val, value);
            return val;
        }

        static bool IsThis(Value const &value);

        static T *Get(Value const &value);
    };
}
