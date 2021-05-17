#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;

namespace runtime {

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data) : data_(data) {}

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const {
        return data_.get();
    }

    ObjectHolder::operator bool() const {
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder& object) {
        if (const String* str = object.TryAs<String>(); str) {
            return str->GetValue().size() != 0;
        }
        if (const Number* num = object.TryAs<Number>(); num) {
            return num->GetValue() != 0;
        }
        if (const Bool* bol = object.TryAs<Bool>(); bol) {
            return bol->GetValue();
        }
        return false;
    }

    void ClassInstance::Print(std::ostream& os, Context& context) {
        if (this->HasMethod("__str__", 0)) {
            this->Call("__str__", {}, context)->Print(os, context);
        } else {
            os << this;
        }
    }

    bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
        const auto* mth_ = this->base_cls_.GetMethod(method);
        if (mth_) {
            return mth_->formal_params.size() == argument_count;
        }
        return false;
    }

    Closure& ClassInstance::Fields() {
        return this->obj_;
        throw std::logic_error("Not implemented");
    }

    const Closure& ClassInstance::Fields() const {
        return this->obj_;
        throw std::logic_error("Not implemented");
    }

    ClassInstance::ClassInstance(const Class& cls) : base_cls_(cls) {}

    ObjectHolder ClassInstance::Call(const std::string& method, const std::vector<ObjectHolder>& actual_args, [[maybe_unused]] Context& context) {
        const auto* mth_ = this->base_cls_.GetMethod(method);
        if (mth_ == nullptr) {
            throw std::runtime_error("Not implemented");
        }
        if (actual_args.size() != mth_->formal_params.size()) {
            throw std::runtime_error("Argument count error");
        }
        Closure cls_;
        int ptr = 0;
        for (const auto& param : mth_->formal_params) {
            cls_.insert({ param, actual_args[ptr++] });
        }
        cls_.insert({ "self", ObjectHolder::Share(*this) });
        return mth_->body->Execute(cls_, context);
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent) : class_name_(name), methods_(std::move(methods)), parrent_class_(parent) {}

    const Method* Class::GetMethod(const std::string& name) const {
        for (size_t ptr = 0; ptr < this->methods_.size(); ptr++) {
            if (this->methods_[ptr].name == name) {
                return &this->methods_[ptr];
            }
        }
        if (this->parrent_class_ != nullptr) {
            const auto* parent_cls = this->parrent_class_->GetMethod(name);
            if (parent_cls) {
                return parent_cls;
            }
        }
        return nullptr;
    }

    const std::string& Class::GetName() const {
        if (!this->class_name_.empty()) {
            return this->class_name_;
        }
        throw std::runtime_error("Not implemented");
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
        os << "Class " << this->GetName();
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs && !rhs) {
            return true;
        } else if (lhs.TryAs<Number>() != nullptr && rhs.TryAs<Number>() != nullptr) {
            return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
        } else if (lhs.TryAs<String>() != nullptr && rhs.TryAs<String>() != nullptr) {
            return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
        } else if (lhs.TryAs<Bool>() != nullptr && rhs.TryAs<Bool>() != nullptr) {
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
        } else if (lhs.TryAs<ClassInstance>() != nullptr) {
            if (lhs.TryAs<ClassInstance>()->HasMethod("__eq__", 1)) {
                return IsTrue(lhs.TryAs<ClassInstance>()->Call("__eq__", { rhs }, context));
            }
        }
        throw std::runtime_error("Cannot compare objects for equality");
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (lhs.TryAs<Number>() != nullptr && rhs.TryAs<Number>() != nullptr) {
            return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
        } else if (lhs.TryAs<String>() != nullptr && rhs.TryAs<String>() != nullptr) {
            const std::string& lhsString = lhs.TryAs<String>()->GetValue();
            const std::string& rhsString = rhs.TryAs<String>()->GetValue();
            return std::lexicographical_compare(lhsString.begin(), lhsString.end(),
                rhsString.begin(), rhsString.end());
        } else if (lhs.TryAs<Bool>() != nullptr && rhs.TryAs<Bool>() != nullptr) {
            return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
        } else if (lhs.TryAs<ClassInstance>() != nullptr) {
            if (lhs.TryAs<ClassInstance>()->HasMethod("__lt__", 1)) {
                return IsTrue(lhs.TryAs<ClassInstance>()->Call("__lt__", { rhs }, context));
            }
        }
        throw std::runtime_error("Cannot compare objects for less"s);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Equal(lhs, rhs, context);
        throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
        throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Greater(lhs, rhs, context);
        throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context);
        throw std::runtime_error("Cannot compare objects for equality"s);
    }

}  // namespace runtime