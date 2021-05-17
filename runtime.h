#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace runtime {

    class Context {
    public:
        virtual std::ostream& GetOutputStream() = 0;
    protected:
        ~Context() = default;
    };

    class Object {
    public:
        virtual ~Object() = default;
        virtual void Print(std::ostream& os, Context& context) = 0;
    };

    class ObjectHolder {
    public:
        ObjectHolder() = default;

        template <typename T>
        [[nodiscard]] static ObjectHolder Own(T&& object) {
            return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
        }

        template <typename T>
        [[nodiscard]] static ObjectHolder Share(T& object) {
            return ObjectHolder(std::shared_ptr<T>(&object, [](auto* /*p*/) { /* do nothing */ }));
        }

        [[nodiscard]] static ObjectHolder None();

        Object& operator*() const;

        Object* operator->() const;

        [[nodiscard]] Object* Get() const;

        template <typename T>
        [[nodiscard]] T* TryAs() const {
            return dynamic_cast<T*>(this->Get());
        }

        explicit operator bool() const;
    private:
        explicit ObjectHolder(std::shared_ptr<Object> data);
        void AssertIsValid() const;
        std::shared_ptr<Object> data_;
    };

    template <typename T>
    class ValueObject : public Object {
    public:
        ValueObject(T v) : value_(v) {}

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override {
            os << value_;
        }

        [[nodiscard]] const T& GetValue() const {
            return value_;
        }

    private:
        T value_;
    };

    using Closure = std::unordered_map<std::string, ObjectHolder>;

    bool IsTrue(const ObjectHolder& object);

    class Executable {
    public:
        virtual ~Executable() = default;
        virtual ObjectHolder Execute(Closure& closure, Context& context) = 0;
    };

    using String = ValueObject<std::string>;

    using Number = ValueObject<int>;

    class Bool : public ValueObject<bool> {
    public:
        using ValueObject<bool>::ValueObject;

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override;
    };

    struct Method {
        std::string name;
        std::vector<std::string> formal_params;
        std::unique_ptr<Executable> body;
    };

    class Class : public Object {
    public:
        explicit Class(std::string name, std::vector<Method> methods, const Class* parent);

        [[nodiscard]] const Method* GetMethod(const std::string& name) const;

        [[nodiscard]] const std::string& GetName() const;

        void Print(std::ostream& os, [[maybe_unused]] Context& context) override;
    private:
        std::string class_name_;
        std::vector<Method> methods_;
        const Class* parrent_class_;
    };

    class ClassInstance : public Object {
    public:
        explicit ClassInstance(const Class& cls);

        void Print(std::ostream& os, Context& context) override;

        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args, Context& context);

        [[nodiscard]] bool HasMethod(const std::string& method, size_t argument_count) const;

        [[nodiscard]] Closure& Fields();
        [[nodiscard]] const Closure& Fields() const;
    private:
        const Class& base_cls_;
        Closure obj_;
    };

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, [[maybe_unused]] Context& context);

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    
    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    
    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    
    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    
    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

    struct DummyContext : Context {
        std::ostream& GetOutputStream() override {
            return output;
        }
        std::ostringstream output;
    };

    class SimpleContext : public runtime::Context {
    public:
        explicit SimpleContext(std::ostream& output)
            : output_(output) {
        }
        std::ostream& GetOutputStream() override {
            return output_;
        }
    private:
        std::ostream& output_;
    };

}  // namespace runtime