#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace parse {

	namespace token_type {

		struct Number {
			int value;
		};

		struct Id {
			std::string value;
		};

		struct Char {
			char value;
		};

		struct String {
			std::string value;
		};

		struct Class {};
		struct Return {};
		struct If {};
		struct Else {};
		struct Def {};
		struct Newline {};
		struct Print {};
		struct Indent {};
		struct Dedent {};
		struct Eof {};
		struct And {};
		struct Or {};
		struct Not {};
		struct Eq {};
		struct NotEq {};
		struct LessOrEq {};
		struct GreaterOrEq {};
		struct None {};
		struct True {};
		struct False {};
	}

	using TokenBase
		= std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
					   token_type::Class, token_type::Return, token_type::If, token_type::Else,
					   token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
					   token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
					   token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
					   token_type::None, token_type::True, token_type::False, token_type::Eof>;

	struct Token : TokenBase {
		using TokenBase::TokenBase;

		template <typename T>
		[[nodiscard]] bool Is() const {
			return std::holds_alternative<T>(*this);
		}

		template <typename T>
		[[nodiscard]] const T& As() const {
			return std::get<T>(*this);
		}

		template <typename T>
		[[nodiscard]] const T* TryAs() const {
			return std::get_if<T>(this);
		}
	};

	bool operator==(const Token& lhs, const Token& rhs);
	bool operator!=(const Token& lhs, const Token& rhs);

	std::ostream& operator<<(std::ostream& os, const Token& rhs);

	class LexerError : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	class Lexer {
	public:
		explicit Lexer(std::istream& input);

		[[nodiscard]] const Token& CurrentToken() const;

		Token NextToken();

		template <typename T>
		const T& Expect() const {
			using namespace std::literals;
			if (!current_token_.Is<T>()){
				throw LexerError("token type error"s);
			}
			return current_token_.As<T>();
		}

		template <typename T, typename U>
		void Expect(const U& value) const {
			using namespace std::literals;
			if (Expect<T>().value != value){
				throw LexerError("token value error"s);
			}
		}

		template <typename T>
		const T& ExpectNext() {
			NextToken();
			return Expect<T>();
		}

		template <typename T, typename U>
		void ExpectNext(const U& value) {
			using namespace std::literals;
			if (ExpectNext<T>().value != value){
				throw LexerError("next token value error"s);
			}
		}

	private:
		bool CheckBeforeParse();
		void HandleCharCases(std::string& string_to_push, const char escaped_char, char ch) const;
		bool ParseKeyword(const std::string& s);
		void ParseString(char c);
		void ParseNumber();
		void ParseIndent();
		void ParseIdentifier();
		void ParseSymbol();
		void ParseToken();

	private:
		std::istream& input_;
		Token current_token_{};
		int count_indent_ = 0;
		int dedent_count_ = 0;
		bool is_start_line_ = true;
		bool is_code_block_ = false;
	};
}
