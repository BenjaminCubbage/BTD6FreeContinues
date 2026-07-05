#pragma once

static void* getIl2CppClass(
        const std::string& assemblyName, 
        const std::string& namespaceName, 
        const std::string& className) {
    void* domain = il2cpp_domain_get();
    void* assembly = il2cpp_domain_assembly_open(domain, assemblyName.c_str());
    void* image = il2cpp_assembly_get_image(assembly);
    return il2cpp_class_from_name(
        image,
        namespaceName.c_str(),
        className.c_str());
}

static void* getIl2CppClassMethod(
        void* klass,
        const std::string& methodName,
        const std::vector<std::string>& argumentTypeNames) {
    while (klass != nullptr) {
        void* iter{};
        void* method{};

        while ((method = il2cpp_class_get_methods(klass, &iter)) != nullptr) {
            if (il2cpp_method_get_name(method) == methodName) {
                if (argumentTypeNames.size() != il2cpp_method_get_param_count(method))
                    continue;

                bool isMatch{ true };
                for (size_t i{}; i < argumentTypeNames.size(); ++i) {
                    void* type = il2cpp_method_get_param(method, (int32_t)i);
                    char* typeName = il2cpp_type_get_name(type);

                    if (typeName != argumentTypeNames[i]) {
                        isMatch = false;
                        break;
                    }

                    il2cpp_free(typeName);
                }
                
                if (isMatch)
                    return method;
            }
        }

        klass = il2cpp_class_get_parent(klass);
    }

    return nullptr;
}

static void* getIl2CppObjectMethod(
    void* obj,
    const std::string& methodName,
    const std::vector<std::string>& argumentTypeNames) {
    return getIl2CppClassMethod(il2cpp_object_get_class(obj), methodName, argumentTypeNames);
}

static void* invokeIl2CppObjectMethod(
    void* obj,
    const std::string& methodName,
    const std::vector<std::string>& argumentTypeNames,
    const std::vector<void*>& arguments) {
    void* method = getIl2CppObjectMethod(
        obj,
        methodName,
        argumentTypeNames);
    
    std::vector<void*> argCopy = arguments;

    /*
        Ugly solution, but we copy args here because they are passed as 
        non-const and it would be annoying if the argument couldn't be 
        accepted as an rvalue.
    */
    void* exc{};
    void* argData = argCopy.data();
    void* result = il2cpp_runtime_invoke(
        method,
        obj,
        &argData,
        &exc);

    return exc == nullptr
        ? result
        : nullptr;
}

static size_t getIl2CppClassFieldOffset(
    void* klass,
    const std::string& fieldName) {
    void* field = il2cpp_class_get_field_from_name(
        klass,
        fieldName.c_str());

    return field != nullptr
        ? il2cpp_field_get_offset(field)
        : size_t(-1);
}

static void* getIl2CppObjectMemberAddr(
    void* obj,
    const std::string& fieldName) {
    size_t offset = getIl2CppClassFieldOffset(
        il2cpp_object_get_class(obj), 
        fieldName);
    
    return offset != size_t(-1)
        ? (uint8_t*)obj + offset
        : nullptr;
}

static void* createKonFuzeInteger(int32_t value) {
    thread_local void* klass{};
    thread_local void* ctor{};

    if (klass == nullptr) {
        klass = getIl2CppClass(
            "Assembly-CSharp.dll",
            "Assets.Scripts.Utils",
            "KonFuze");
        ctor = getIl2CppClassMethod(
            klass, 
            "op_Implicit", 
            { "System.Int32" });
    }

    if (klass == nullptr || ctor == nullptr)
        return nullptr;

    void* exception{};
    void* arg = &value;
    return il2cpp_runtime_invoke(ctor, nullptr, &arg, &exception);
}