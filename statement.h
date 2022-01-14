#pragma once
#include "runtime.h"

#include <functional>

namespace ast{

	using Statement = runtime::Executable;

	template <typename T>
	class ValueStatement : public Statement {
	public:
		explicit ValueStatement(T v)
			: value_(std::move(v)){
		}

		runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure& closure,
				[[maybe_unused]] runtime::Context& context) override {
			return runtime::ObjectHolder::Share(value_);
		}

	private:
		T value_;
	};

	using NumericConst = ValueStatement<runtime::Number>;
	using StringConst = ValueStatement<runtime::String>;
	using BoolConst = ValueStatement<runtime::Bool>;

	class VariableValue : public Statement {
	public:
		explicit VariableValue(const std::string& var_name);
		explicit VariableValue(std::vector<std::string> dotted_ids);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::vector<std::string> dotted_ids_;
	};

	class Assignment : public Statement {
	public:
		Assignment(std::string var_name, std::unique_ptr<Statement> rv);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::string var_name_;
		std::unique_ptr<Statement> rv_;
	};

	class FieldAssignment : public Statement {
	public:
		FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		VariableValue object_;
		std::string field_name_;
		std::unique_ptr<Statement> rv_;
	};

	class None : public Statement {
	public:
		runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure& closure,
				[[maybe_unused]] runtime::Context& context) override {
			return {};
		}
	};

	class Print : public Statement {
	public:
		explicit Print(std::unique_ptr<Statement> argument);
		explicit Print(std::vector<std::unique_ptr<Statement>> args);
		static std::unique_ptr<Print> Variable(const std::string& name);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::vector<std::unique_ptr<Statement>> args_;
	};

	class MethodCall : public Statement {
	public:
		MethodCall(std::unique_ptr<Statement> object, std::string method,
				std::vector<std::unique_ptr<Statement>> args);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::unique_ptr<Statement> object_;
		std::string method_;
		std::vector<std::unique_ptr<Statement>> args_;
	};

	class NewInstance : public Statement {
	public:
		explicit NewInstance(const runtime::Class& cls);
		NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		runtime::ClassInstance instance_;
		std::vector<std::unique_ptr<Statement>> args_;
	};

	class UnaryOperation : public Statement {
	public:
		explicit UnaryOperation(std::unique_ptr<Statement> argument)
			: argument_(std::move(argument))
		{}

	protected:
		std::unique_ptr<Statement> argument_;
	};

	class Stringify : public UnaryOperation {
	public:
		using UnaryOperation::UnaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class BinaryOperation : public Statement {
	public:
		BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs)
			: lhs_(std::move(lhs))
			, rhs_(std::move(rhs))
		{}

	protected:
		std::unique_ptr<Statement> lhs_;
		std::unique_ptr<Statement> rhs_;
	};

	class Add : public BinaryOperation {
	public:
		using BinaryOperation::BinaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class Sub : public BinaryOperation {
	public:
		using BinaryOperation::BinaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class Mult : public BinaryOperation {
	public:
		using BinaryOperation::BinaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class Div : public BinaryOperation {
	public:
		using BinaryOperation::BinaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class Or : public BinaryOperation {
	public:
		using BinaryOperation::BinaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class And : public BinaryOperation {
	public:
		using BinaryOperation::BinaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class Not : public UnaryOperation {
	public:
		using UnaryOperation::UnaryOperation;
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
	};

	class Compound : public Statement {
	public:
		template <typename... Args>
		explicit Compound(Args&&... args) {
			if constexpr (sizeof...(args) != 0) {
				AddStatementInVector(std::forward<Args>(args)...);
			}
		}

		void AddStatement(std::unique_ptr<Statement> stmt) {
			AddStatementInVector(std::move(stmt));
		}

		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		template <typename T0, typename... Ts>
		void AddStatementInVector(T0&& v0, Ts&&... vs) {
			statements_.push_back(std::forward<T0>(v0));
			if constexpr (sizeof...(vs) != 0) {
				AddStatementInVector(std::forward<Ts>(vs)...);
			}
		}

		std::vector<std::unique_ptr<Statement>> statements_;
	};

	class MethodBody : public Statement {
	public:
		explicit MethodBody(std::unique_ptr<Statement>&& body);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::unique_ptr<Statement> body_;
	};

	class Return : public Statement {
	public:
		explicit Return(std::unique_ptr<Statement> statement)
			: statement_(std::move(statement))
		{}

		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::unique_ptr<Statement> statement_;
	};

	class ClassDefinition : public Statement {
	public:
		explicit ClassDefinition(runtime::ObjectHolder cls);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		runtime::ObjectHolder cls_;
	};

	class IfElse : public Statement {
	public:
		IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
				std::unique_ptr<Statement> else_body);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		std::unique_ptr<Statement> condition_;
		std::unique_ptr<Statement> if_body_;
		std::unique_ptr<Statement> else_body_;
	};

	class Comparison : public BinaryOperation {
	public:
		using Comparator = std::function<bool(const runtime::ObjectHolder&,
				  const runtime::ObjectHolder&, runtime::Context&)>;
		Comparison(Comparator cmp, std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);
		runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

	private:
		Comparator cmp_;
	};
}
