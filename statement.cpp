#include "statement.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <iostream>

using namespace std;

namespace ast {

    using runtime::Closure;
    using runtime::Context;
    using runtime::ObjectHolder;

    namespace {
        const string ADD_METHOD = "__add__"s;
        const string INIT_METHOD = "__init__"s;
    }  // namespace ast

    ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
        closure[this->var_] = this->rv_->Execute(closure, context);
        return closure.at(this->var_);
        throw std::runtime_error("Not in list");
    }

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : var_(std::move(var)), rv_(std::move(rv)) {}

    VariableValue::VariableValue(const std::string& var_name) {
        this->var_names_.push_back(var_name);
    }

    VariableValue::VariableValue(std::vector<std::string> dotted_ids) {
        this->var_names_ = dotted_ids;
    }

    ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]]Context& context) {
        auto* cosulya = &closure;
        for (size_t ptr = 0; ptr < this->var_names_.size(); ptr++) {
            if (cosulya->count(this->var_names_[ptr]) == 1) {
                if (ptr + 1 < this->var_names_.size()) {
                    cosulya = &((*cosulya)[this->var_names_[ptr]].TryAs<runtime::ClassInstance>()->Fields());
                } else {
                    return (*cosulya)[this->var_names_[ptr]];
                }
            }
        }
        throw std::runtime_error("Not in list");
    }

    unique_ptr<Print> Print::Variable(const std::string& name) {
        vector<unique_ptr<Statement>> args;
        args.push_back(std::make_unique<VariableValue>(name));
        return std::make_unique<Print>(std::move(args));
    }

    Print::Print(unique_ptr<Statement> argument) {
        this->args_.push_back(std::move(argument));
    }

    Print::Print(vector<unique_ptr<Statement>> args) : args_(std::move(args)) {}

    ObjectHolder Print::Execute(Closure& closure, Context& context) {
        for (size_t ptr = 0; ptr < this->args_.size(); ptr++) {
            auto obj = this->args_[ptr].get()->Execute(closure, context);
            if (obj) {
                obj->Print(context.GetOutputStream(), context);
            } else {
                context.GetOutputStream() << "None";
            }
            if (ptr < this->args_.size() - 1) {
                context.GetOutputStream() << " ";
            }
        }
        context.GetOutputStream() << "\n";
        return {};
    }

    MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
        std::vector<std::unique_ptr<Statement>> args)
        : object_(std::move(object)), method_(std::move(method)), args_(std::move(args)) {
    }

    ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
        if (runtime::ClassInstance* instance = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
            instance != nullptr) {
            if (instance->HasMethod(method_, args_.size())) {
                std::vector<ObjectHolder> actualArgs;
                for (const auto& arg : args_) {
                    actualArgs.push_back(arg->Execute(closure, context));
                }
                return instance->Call(method_, actualArgs, context);
            }
        }
        throw std::runtime_error("Can not call method " + method_);
    }

    ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
        auto args = UnaryOperation::arg_->Execute(closure, context);
        stringstream tmp_str;
        if (args) {
            args->Print(tmp_str, context);
        } else {
            tmp_str << "None";
        }
        return ObjectHolder::Own(runtime::String(tmp_str.str()));
    }

    ObjectHolder Add::Execute(Closure& closure, Context& context) {
        auto lhs = std::move(BinaryOperation::lhs_)->Execute(closure, context);
        auto rhs = std::move(BinaryOperation::rhs_)->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() != nullptr && rhs.TryAs<runtime::Number>() != nullptr) {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() + rhs.TryAs<runtime::Number>()->GetValue()));
        } else if (lhs.TryAs<runtime::String>() != nullptr && rhs.TryAs<runtime::String>() != nullptr) {
            return ObjectHolder::Own(runtime::String(lhs.TryAs<runtime::String>()->GetValue() + rhs.TryAs<runtime::String>()->GetValue()));
        } else if (lhs.TryAs<runtime::ClassInstance>() != nullptr) {
            if (lhs.TryAs<runtime::ClassInstance>()->HasMethod(ast::ADD_METHOD, 1)) {
                return (lhs.TryAs<runtime::ClassInstance>()->Call(ast::ADD_METHOD, { rhs }, context));
            }
        }
        throw runtime_error("Type add error");
    }

    ObjectHolder Sub::Execute(Closure& closure, Context& context) {
        auto lhs = std::move(BinaryOperation::lhs_)->Execute(closure, context);
        auto rhs = std::move(BinaryOperation::rhs_)->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() != nullptr && rhs.TryAs<runtime::Number>() != nullptr) {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() - rhs.TryAs<runtime::Number>()->GetValue()));
        }
        throw runtime_error("Type sub error");
    }

    ObjectHolder Mult::Execute(Closure& closure, Context& context) {
        auto lhs = std::move(BinaryOperation::lhs_)->Execute(closure, context);
        auto rhs = std::move(BinaryOperation::rhs_)->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() != nullptr && rhs.TryAs<runtime::Number>() != nullptr) {
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() * rhs.TryAs<runtime::Number>()->GetValue()));
        }
        throw runtime_error("Type multi error");
    }

    ObjectHolder Div::Execute(Closure& closure, Context& context) {
        auto lhs = std::move(BinaryOperation::lhs_)->Execute(closure, context);
        auto rhs = std::move(BinaryOperation::rhs_)->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() != nullptr && rhs.TryAs<runtime::Number>() != nullptr) {
            if (rhs.TryAs<runtime::Number>()->GetValue() == 0) {
                throw runtime_error("Divided by zero");
            }
            return ObjectHolder::Own(runtime::Number(lhs.TryAs<runtime::Number>()->GetValue() / rhs.TryAs<runtime::Number>()->GetValue()));
        }
        throw runtime_error("Type sun error");
    }

    ObjectHolder Compound::Execute(Closure& closure, Context& context) {
        for (size_t ptr = 0; ptr < this->args_.size(); ptr++) {
            this->args_[ptr]->Execute(closure, context);
        }
        return ObjectHolder::None();
    }

    ObjectHolder Return::Execute(Closure& closure, Context& context) {
        auto rnrned = this->st_->Execute(closure, context);;
        throw ObjRet(rnrned);
    }

    ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(std::move(cls)) {}

    ObjectHolder ClassDefinition::Execute(Closure& closure, Context& context) {
        NewInstance inst(*cls_.TryAs<runtime::Class>());
        closure[cls_.TryAs<runtime::Class>()->GetName()] = inst.Execute(closure, context);
        return closure[cls_.TryAs<runtime::Class>()->GetName()];
    }

    FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv) : str_name_(std::move(field_name)), obj_(std::move(object)), rv_(std::move(rv)) {}

    ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
        ObjectHolder tmp = this->obj_.Execute(closure, context);
        if (!tmp.TryAs<runtime::ClassInstance>()) {
            throw std::runtime_error("Some data error");
        }
        tmp.TryAs<runtime::ClassInstance>()->Fields()[this->str_name_] = this->rv_->Execute(closure, context);
        return  tmp.TryAs<runtime::ClassInstance>()->Fields().at(this->str_name_);
    }

    IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> ifBody, std::unique_ptr<Statement> elseBody) : cond_(std::move(condition)), ifb_(std::move(ifBody)), elseb_(std::move(elseBody)) {}

    ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
        if (runtime::IsTrue(this->cond_->Execute(closure, context))) {
            return this->ifb_->Execute(closure, context);
        } else {
            if (this->elseb_) {
                return this->elseb_->Execute(closure, context);
            }
        }
        return ObjectHolder::None();
    }

    ObjectHolder Or::Execute(Closure& closure, Context& context) {
        auto lhs = (BinaryOperation::lhs_)->Execute(closure, context);
        auto rhs = (BinaryOperation::rhs_)->Execute(closure, context);
        if (!runtime::IsTrue(lhs)) {
            return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(rhs)));
        }
        return ObjectHolder::Own(runtime::Bool(true));
        throw runtime_error("Type or error");
    }

    ObjectHolder And::Execute(Closure& closure, Context& context) {
        auto lhs = (BinaryOperation::lhs_)->Execute(closure, context);
        auto rhs = (BinaryOperation::rhs_)->Execute(closure, context);
        if (runtime::IsTrue(lhs)) {
            return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(rhs)));
        }
        return ObjectHolder::Own(runtime::Bool(false));
        throw runtime_error("Type and error");
    }

    ObjectHolder Not::Execute(Closure& closure, Context& context) {
        auto lhs = (UnaryOperation::arg_)->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(!runtime::IsTrue(lhs)));
    }

    Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs) : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {
    }

    ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
        bool result = cmp_(this->lhs_->Execute(closure, context), this->rhs_->Execute(closure, context), context);
        return ObjectHolder::Own(runtime::Bool(result));
    }

    NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) : cls_(class_), args_(std::move(args)) {}

    NewInstance::NewInstance(const runtime::Class& class_) : cls_(class_) {}

    ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
        runtime::ClassInstance copy_cls(this->cls_);
        auto tmp_cls_inst = ObjectHolder::Own(std::move(copy_cls));
        if (tmp_cls_inst.TryAs<runtime::ClassInstance>()->HasMethod(ast::INIT_METHOD, this->args_.size())) {
            std::vector<runtime::ObjectHolder> new_arg;
            for (const auto& s : this->args_) {
                new_arg.push_back(s.get()->Execute(closure, context));
            }
            tmp_cls_inst.TryAs<runtime::ClassInstance>()->Call(INIT_METHOD, new_arg, context);
            
        }
        return tmp_cls_inst;
    }

    MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(std::move(body)) {}

    ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
        try {
            auto a = this->body_->Execute(closure, context);
        } catch (ObjRet& e) {
            return e.Get_ObjHldr();
        }
        return ObjectHolder::None();
    }

}  // namespace ast