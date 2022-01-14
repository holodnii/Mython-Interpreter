#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;

namespace runtime {

	namespace detail {

		static const std::string SELF("self"s);
		static const std::string TRUE("True"s);
		static const std::string FALSE("False"s);
		static const std::string CLASS("Class"s);
		static const std::string STR("__str__"s);
		static const std::string EQ("__eq__"s);
		static const std::string LT("__lt__"s);
	}

	ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
		: data_(std::move(data)) {
	}

	void ObjectHolder::AssertIsValid() const {
		assert(data_ != nullptr);
	}

	ObjectHolder ObjectHolder::Share(Object& object) {
		return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
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
		if (object){
			if (const Number* num = object.TryAs<Number>()){
				return num->GetValue();
			}
			if (const String* str = object.TryAs<String>()){
				return !str->GetValue().empty();
			}
			if (const Bool* b = object.TryAs<Bool>()){
				return b->GetValue();
			}
		}
		return false;
	}

	const Method* ClassInstance::TryMethod(const std::string& method, size_t argument_count) const {
		return cls_.GetMethod(method, argument_count);
	}

	const Method* ClassInstance::GetMethod(const std::string& method, size_t argument_count) const{
		if (const auto* ptr_method = cls_.GetMethod(method, argument_count)){
			return ptr_method;
		}
		std::stringstream ss;
		ss << "Unknown method name: "sv << method;
		throw std::runtime_error(ss.str());
	}

	void ClassInstance::Print(std::ostream& os, Context& context) {
		if (const auto* ptr_method = TryMethod(detail::STR, 0)){
			ObjectHolder object_holder = Call(ptr_method, {}, context);
			object_holder.Get()->Print(os, context);
		}else{
			os << this;
		}
	}

	bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
		return TryMethod(method, argument_count) != nullptr;
	}

	Closure& ClassInstance::Fields() {
		return closure_;
	}

	const Closure& ClassInstance::Fields() const {
		return closure_;
	}

	ClassInstance::ClassInstance(const Class& cls)
		: cls_(cls)
	{}

	Closure ClassInstance::CreateLocalClosure(
			const std::vector<std::string>& formal_params,
			const std::vector<ObjectHolder>& actual_args){
		assert(formal_params.size() == actual_args.size());
		Closure closure;
		closure.emplace(runtime::detail::SELF, ObjectHolder::Share(*this));
		for (size_t i = 0; i < formal_params.size(); ++i){
			closure.emplace(formal_params.at(i), actual_args.at(i));
		}

		return closure;
	}

	ObjectHolder ClassInstance::Call(const Method* method, const std::vector<ObjectHolder>& actual_args, Context& context){
		Closure local_closure = CreateLocalClosure(method->formal_params, actual_args);
		return method->body.get()->Execute(local_closure, context);
	}

	ObjectHolder ClassInstance::Call(const std::string& method,
									 const std::vector<ObjectHolder>& actual_args,
									 Context& context) {
		const auto* ptr_method = GetMethod(method, actual_args.size());
		return Call(ptr_method, actual_args, context);
	}

	Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
		: name_(name)
		, methods_(std::move(methods))
		, parent_(parent)
	{}

	[[nodiscard]] const Method* Class::GetMethod(const std::string& name) const {
		auto it = std::find_if(methods_.begin(), methods_.end(),
				[&name](const auto& value) { return value.name == name; });
		if (it == methods_.end()){
			if (parent_){
				return parent_->GetMethod(name);
			}
			return nullptr;
		}
		return &(*it);
	}

	[[nodiscard]] const Method* Class::GetMethod(const std::string& name, size_t args_count) const{
		const Method* method = GetMethod(name);
		if (method != nullptr && method->formal_params.size() == args_count){
			return method;
		}
		return nullptr;
	}

	[[nodiscard]] const std::string& Class::GetName() const {
		return name_;
	}

	void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
		os << runtime::detail::CLASS << " "s << name_;
	}

	void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
		os << (GetValue() ? runtime::detail::TRUE : runtime::detail::FALSE);
	}

	template <typename Compare>
	bool MakeComparison(const ObjectHolder& lhs, const ObjectHolder& rhs,
			Context& context, const std::string& func_name, Compare cmp){
		if (lhs && rhs){
			if (auto* ptr_cls = lhs.TryAs<ClassInstance>()){
				return IsTrue(ptr_cls->Call(func_name, {rhs}, context));
			}

			if (lhs.IsType<String>() && rhs.IsType<String>()){
				return cmp(lhs.TryAs<String>()->GetValue(), rhs.TryAs<String>()->GetValue());
			}
			if (lhs.IsType<Number>() && rhs.IsType<Number>()){
				return cmp(lhs.TryAs<Number>()->GetValue(), rhs.TryAs<Number>()->GetValue());
			}
			if (lhs.IsType<Bool>() && rhs.IsType<Bool>()){
				return cmp(lhs.TryAs<Bool>()->GetValue(), rhs.TryAs<Bool>()->GetValue());
			}
		}
		throw std::runtime_error("Cannot compare objects for "s + func_name);
	}

	bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		if (!lhs && !rhs){
			return true;
		}

		return MakeComparison(lhs, rhs, context, runtime::detail::EQ, std::equal_to{});
	}

	bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return MakeComparison(lhs, rhs, context, runtime::detail::LT, std::less{});
	}

	bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Equal(lhs, rhs, context);
	}

	bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
	}

	bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Greater(lhs, rhs, context);
	}

	bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
		return !Less(lhs, rhs, context);
	}
}
