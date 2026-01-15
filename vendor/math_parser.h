#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <cctype>
#include <functional>
#include <optional>
#include <algorithm>
#include <iostream>
#include <utility>




namespace MathParser
{
	namespace detail
	{
		namespace CONSTANTS
		{
			constexpr double PI  = 3.14159265358979323846;
			constexpr double TAU = 6.28318530717958647692;
			constexpr double e   = 2.71828182845904523536;
			constexpr double PHI = 1.61803398874989484820; // (1+√5)/2

			constexpr double DEG_TO_RAD = PI / 180.0;
			constexpr double RAD_TO_DEG = 180.0 / PI;

			constexpr double EPSILON = 1e-6; // Internal, not implemented
		}

		namespace STANDARD_FUNCTIONS
		{
			static const std::unordered_map<std::string, double(*)(double)> sf
			{
				{"exp",    std::exp},
				{"ln",     std::log},
				{"log",    std::log10},

				{"sin",    std::sin},
				{"cos",    std::cos},
				{"tan",    std::tan},

				{"asin",   std::asin},
				{"arcsin", std::asin},
				{"csc",		[](double x)  { return 1.0 / std::sin(x);  }},
				{"arccsc",  [](double x)  { return std::asin(1.0 / x); }},
				{"acsc",    [](double x)  { return std::asin(1.0 / x); }},

				{"acos",   std::acos},
				{"arccos", std::acos},
				{"sec",		[](double x)  { return 1.0 / std::cos(x);  }},
				{"arcsec",  [](double x)  { return std::acos(1.0 / x); }},
				{"asec",    [](double x)  { return std::acos(1.0 / x); }},

				{"atan",   std::atan},
				{"arctan", std::atan},
				{"cot",     [](double x)  { return 1.0 / std::tan(x);  }},
				{"arccot",  [](double x)  { return std::atan(1.0 / x); }},
				{"acot",    [](double x)  { return std::atan(1.0 / x); }},

				{"sinh",  std::sinh},
				{"cosh",  std::cosh},
				{"tanh",  std::tanh},
				{"asinh", std::asinh},
				{"acosh", std::acosh},
				{"atanh", std::atanh},

				{"sqrt",   std::sqrt},
				{"cbrt",   std::cbrt},

				{"abs",   std::fabs},
				{"floor", std::floor},
				{"ceil",  std::ceil},
				{"round", std::round},
				{"trunc", std::trunc}

				// Also supported:
				// min(a,b)
				// max(a,b)
				// hypot(x,y)
				// clamp(x, a, b)
			};
		}


		using VarTable = std::unordered_map<std::string, double>;

		class Expression
		{
		public:
			virtual ~Expression() = default;
			virtual double Eval(const VarTable& vars) const = 0;
			virtual void CollectVars(std::unordered_set<std::string>& out) const = 0;
		};

		using ExprPtr = std::unique_ptr<Expression>;

		template<typename T, typename... Args>
		constexpr ExprPtr CreateExpr(Args&&... args)
		{
			return std::make_unique<T>(std::forward<Args>(args)...);
		}


		// ============================================================================
		// ============================================================================


		enum class RelOp { EQ, LT, LE, GT, GE };

		struct Relation
		{
			RelOp op;
			ExprPtr LHS;
			ExprPtr RHS;

			Relation(RelOp o, ExprPtr L, ExprPtr R)
				: op(o), LHS(std::move(L)), RHS(std::move(R)) {}

			bool evalRel(const VarTable& vars) const
			{
				double a = LHS->Eval(vars);
				double b = RHS->Eval(vars);

				switch (op)
				{
					case RelOp::EQ: return std::abs(a - b) < CONSTANTS::EPSILON;
					case RelOp::LT: return a < b;
					case RelOp::LE: return a <= b;
					case RelOp::GT: return a > b;
					case RelOp::GE: return a >= b;
				}
				return false;
			}
		};

		using RelPtr = std::unique_ptr<Relation>;

		template<typename T, typename... Args>
		constexpr RelPtr CreateRel(Args&&... args)
		{
			return std::make_unique<T>(std::forward<Args>(args)...);
		}




		// ============================================================================
		// ============================ Expression Types ==============================
		// ============================================================================



		struct Constant : Expression
		{
			double value;
			explicit Constant(double v) : value(v) {};
			double Eval(const VarTable&) const override
			{
				return value;
			}
			void CollectVars(std::unordered_set<std::string>&) const override {};
		};

		class Variable : public Expression
		{
		public:
			explicit Variable(std::string name) : m_Name(std::move(name)) {};

			double Eval(const VarTable& vars) const override
			{
				auto it = vars.find(m_Name);
				if (it == vars.end())
					throw std::runtime_error("Undefined variable: " + m_Name);
				return it->second;
			}

			void CollectVars(std::unordered_set<std::string>& out) const override
			{
				out.insert(m_Name);
			}

		private:
			std::string m_Name;
		};

		struct Binary : Expression
		{
			char op;
			ExprPtr LHS;
			ExprPtr RHS;

			Binary(char o, ExprPtr L, ExprPtr R)
				: op(o), LHS(std::move(L)), RHS(std::move(R)) {};

			double Eval(const VarTable& vars) const override
			{
				double a = LHS->Eval(vars);
				double b = RHS->Eval(vars);

				switch (op)
				{
					case '+': return a + b;
					case '-': return a - b;
					case '*': return a * b;
					case '/': return a / b;
					case '^': return std::pow(a, b);
					default: throw std::runtime_error("Undefined operator. Valid Operators are: '+', '-', '*', '/', '^'");
				}
			}

			void CollectVars(std::unordered_set<std::string>& out) const override
			{
				LHS->CollectVars(out);
				RHS->CollectVars(out);
			}
		};

		struct Unary : Expression
		{
			char op;
			ExprPtr expr;

			Unary(char o, ExprPtr e) : op(o), expr(std::move(e)) {};

			double Eval(const VarTable& vars) const override
			{
				double v = expr->Eval(vars);
				return (op == '-') ? -v : v;
			}

			void CollectVars(std::unordered_set<std::string>& out) const override
			{
				expr->CollectVars(out);
			}
		};

		struct FunctionDef
		{
			std::string name;
			std::vector<std::string> params;
			ExprPtr body;
		};


		class FunctionEngine
		{
		private:
			std::unordered_map<std::string, FunctionDef> m_Functions;
		public:

			void AddFunction(FunctionDef def)
			{
				std::string key = def.name;
				m_Functions[key] = std::move(def);
			}

			const FunctionDef* FindFunction(const std::string& name) const
			{
				auto it = m_Functions.find(name);
				return it == m_Functions.end() ? nullptr : &it->second;
			}

			bool HasFunction(const std::string& name) const
			{
				return m_Functions.contains(name);

			}

		};

		struct Function : Expression
		{
			std::string name;
			std::vector<ExprPtr> args;
			std::shared_ptr<FunctionEngine> engine;

			Function(std::string n,
					 std::vector<ExprPtr> a,
					 std::shared_ptr<FunctionEngine> e)
				: name(std::move(n)), args(std::move(a)), engine(std::move(e)) {}


			double Eval(const VarTable& vars) const override
			{
				if (const FunctionDef* def = engine->FindFunction(name))
				{
					if (def->params.size() != args.size())
						throw std::runtime_error("Argument count mismatch");

					VarTable local = vars;
					for (size_t i = 0; i < args.size(); ++i)
						local[def->params[i]] = args[i]->Eval(vars);

					return def->body->Eval(local);
				}

				// ================================================================
				// ================= Special functionality handling ===============
				// ================================================================

				if (name == "log")
				{
					if (args.size() == 1)
					{
						double x = args[0]->Eval(vars);
						return std::log10(x);
					}
					if (args.size() == 2)
					{
						double x = args[0]->Eval(vars);
						double b = args[1]->Eval(vars);
						return std::log(x) / std::log(b);
					}
					throw std::runtime_error("log() expects 1 or 2 arguments");
				}


				if (name == "min")
				{
					if (args.size() != 2)
						throw std::runtime_error("min() expects exactly 2 arguments");

					double a = args[0]->Eval(vars);
					double b = args[1]->Eval(vars);
					return std::min(a, b);
				}

				if (name == "max")
				{
					if (args.size() != 2)
						throw std::runtime_error("max() expects exactly 2 arguments");

					double a = args[0]->Eval(vars);
					double b = args[1]->Eval(vars);
					return std::max(a, b);
				}
				if (name == "hypot") // sqrt(x^2 + y^2)
				{
					if (args.size() != 2)
						throw std::runtime_error("hypot() expects exactly 2 arguments");

					double x = args[0]->Eval(vars);
					double y = args[1]->Eval(vars);
					return std::hypot(x, y);
				}
				if (name == "clamp") // min(max(x, a), b)
				{
					if (args.size() != 3)
						throw std::runtime_error("clamp() expects exactly 3 arguments");

					double x = args[0]->Eval(vars);
					double a = args[1]->Eval(vars);
					double b = args[2]->Eval(vars);

					if (b < a)
						throw std::runtime_error("clamp() is only defined when the third argument is >= the second argument");

					return std::clamp(x, a, b);
				}
				if (name == "sign" || name == "sgn")
				{
					if (args.size() != 1)
						throw std::runtime_error("sign() or sgn() expect exactly 1 argument");

					double x = args[0]->Eval(vars);

					if (x > 0) return  1.0;
					if (x < 0) return -1.0;
					else	   return 0.0;
				}

				if (args.size() != 1)
					throw std::runtime_error("Function expects exactly 1 argument");


				// ================================================================
				// ================================================================


				// one arg built-in functions
				if (STANDARD_FUNCTIONS::sf.contains(name))
				{
					if (args.size() != 1)
						throw std::runtime_error(name + "() expects exactly 1 argument");

					double v = args[0]->Eval(vars);
					return STANDARD_FUNCTIONS::sf.at(name)(v);
				}


				throw std::runtime_error("Unknown function: " + name);
			}


			void CollectVars(std::unordered_set<std::string>& out) const override
			{
				for (auto& a : args)
					a->CollectVars(out);
			}
		};

		using ParseResult = std::variant<std::monostate, ExprPtr, RelPtr, FunctionDef>;




		// ======================================================================================
		// ===================================== PARSER =========================================
		// ======================================================================================



		class Parser
		{
		private:
			std::string m_Input;
			size_t m_Pos = 0;
			ExprPtr m_LastExpression;
			std::optional<FunctionDef> m_LastFunctionDef;
			std::shared_ptr<FunctionEngine> m_FunctionEngine;


		public:
			explicit Parser(const std::string& input,
							std::shared_ptr<FunctionEngine> engine)
				: m_Input(input), m_FunctionEngine(std::move(engine)) {}

			ParseResult Parse()
			{
				m_Pos = 0;
				m_LastExpression.reset();
				m_LastFunctionDef.reset();
				m_LastAtom = LastAtom::None;
				skipWhitespace();

				if (ParseFunctionDef())
				{
					skipWhitespace();
					if (m_Pos != m_Input.size())
						throw std::runtime_error("Unexpected trailing characters after function definition");
					return std::move(*m_LastFunctionDef);
				}

				auto rel = ParseRelation();
				skipWhitespace();

				if (m_Pos != m_Input.size())
					throw std::runtime_error("Unexpected trailing characters");

				if (rel) return rel;
				return std::move(m_LastExpression);
			}


		private:
			enum class LastAtom
			{
				None,
				NumberOrParen,  // 2, (x+1)
				Identifier      // x, g
			};

			LastAtom m_LastAtom = LastAtom::None;

			// Parsing functions ====================================================

			ExprPtr ParseExpression()
			{

				auto left = ParseTerm();

				while (true)
				{
					skipWhitespace();

					if (match('+'))
						left = CreateExpr<Binary>('+', std::move(left), ParseTerm());
					else if (match('-'))
						left = CreateExpr<Binary>('-', std::move(left), ParseTerm());
					else
						break;
				}

				return left;
			}

			ExprPtr ParseTerm()
			{
				auto left = ParseUnary();

				while (true)
				{
					skipWhitespace();

					if (match('*'))
						left = CreateExpr<Binary>('*', std::move(left), ParseUnary());
					else if (match('/'))
						left = CreateExpr<Binary>('/', std::move(left), ParseUnary());
					else if (startsPrimary(peek()) || peek() == '(')
					{
						left = CreateExpr<Binary>('*', std::move(left), ParseUnary());
					}
					else
						break;
				}

				return left;
			}

			ExprPtr ParsePower()
			{
				auto left = ParsePrimary();

				skipWhitespace();
				if (match('^'))
				{
					// right-associative
					auto right = ParsePower();
					return CreateExpr<Binary>('^', std::move(left), std::move(right));
				}

				return left;
			}

			ExprPtr ParseUnary()
			{
				skipWhitespace();

				// unary + / -
				if (match('+'))
					return ParseUnary();

				if (match('-'))
					return CreateExpr<Unary>('-', ParseUnary());


				return ParsePower();

			}

			ExprPtr ParsePrimary()
			{
				skipWhitespace();

				// number
				if (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.')
				{
					size_t start = m_Pos;
					while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.')
						advance();

					double value = std::stod(m_Input.substr(start, m_Pos - start));
					m_LastAtom = LastAtom::NumberOrParen;
					return CreateExpr<Constant>(value);
				}

				// parenthesized expression
				if (match('('))
				{
					auto expr = ParseExpression();
					if (!match(')'))
						throw std::runtime_error("Expected ')'");
					m_LastAtom = LastAtom::NumberOrParen;
					return expr;
				}
				// function without parentheses: sin x, sin 2x, sin x^2
				if (std::isalpha(static_cast<unsigned char>(peek())))
				{
					size_t start = m_Pos;
					while (std::isalpha(static_cast<unsigned char>(peek())))
						advance();

					std::string name = m_Input.substr(start, m_Pos - start);

					// built-in constants
					if (name == "pi" || name == "PI")
					{
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Constant>(CONSTANTS::PI);
					}
					if (name == "tau" || name == "TAU")
					{
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Constant>(CONSTANTS::TAU);
					}
					if (name == "phi" || name == "PHI")
					{
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Constant>(CONSTANTS::PHI);
					}

					if (name == "e")
					{
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Constant>(CONSTANTS::e);
					}
					if (name == "DEG_TO_RAD")
					{
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Constant>(CONSTANTS::DEG_TO_RAD);
					}
					if (name == "RAD_TO_DEG")
					{
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Constant>(CONSTANTS::RAD_TO_DEG);
					}



					skipWhitespace();

					// function call with parentheses
					// identifier followed by '(' : could be function call OR implicit multiplication
					if (peek() == '(')
					{
						// If it's a known function, always treat as a call.
						// If it's not known but has length > 1 (e.g. unknown(1)), treat as a call anyway
						// so it throws "Unknown function: unknown" rather than "Undefined variable: unknown".
						if (isFunctionName(name) || name.size() > 1)
						{
							match('('); // consume '('
							auto args = ParseArgumentList();
							m_LastAtom = LastAtom::NumberOrParen; // f(x) behaves like an atom
							return CreateExpr<Function>(name, std::move(args), m_FunctionEngine);
						}

						// Otherwise, single-letter like x(y): treat as variable 'x' and let ParseTerm
						// handle implicit multiplication with the upcoming '('.
					}

					// function without parentheses: sin x, cos x, etc.
					if (isFunctionName(name))
					{
						std::vector<ExprPtr> args;
						args.push_back(ParseUnary());
						m_LastAtom = LastAtom::NumberOrParen;
						return CreateExpr<Function>(name, std::move(args), m_FunctionEngine);
					}

					// otherwise it's a variable
					m_LastAtom = LastAtom::Identifier;
					return CreateExpr<Variable>(name);

				}
				throw std::runtime_error("Unexpected token");

			}


			std::vector<ExprPtr> ParseArgumentList()
			{
				std::vector<ExprPtr> args;

				if (match(')'))
					return args;

				while (true)
				{
					args.push_back(ParseExpression());

					skipWhitespace();
					if (match(')'))
						break;

					skipWhitespace();
					if (!match(','))
						throw std::runtime_error("Expected ',' or ')'");
				}

				return args;
			}

			RelPtr ParseRelation()
			{
				auto left = ParseExpression();
				m_LastExpression = std::move(left);

				skipWhitespace();

				if (match('='))
					return CreateRel<Relation>(RelOp::EQ, std::move(m_LastExpression), ParseExpression());

				if (match('<'))
				{
					if (match('='))
						return CreateRel<Relation>(RelOp::LE, std::move(m_LastExpression), ParseExpression());
					return CreateRel<Relation>(RelOp::LT, std::move(m_LastExpression), ParseExpression());
				}

				if (match('>'))
				{
					if (match('='))
						return CreateRel<Relation>(RelOp::GE, std::move(m_LastExpression), ParseExpression());
					return CreateRel<Relation>(RelOp::GT, std::move(m_LastExpression), ParseExpression());
				}

				return nullptr;
			}

			bool ParseFunctionDef()
			{
				size_t startPos = m_Pos;

				if (!std::isalpha(static_cast<unsigned char>(peek())))
					return false;

				std::string name;
				while (std::isalpha(static_cast<unsigned char>(peek())))
					name += advance();

				skipWhitespace();
				if (!match('('))
				{
					m_Pos = startPos;
					return false;
				}

				std::vector<std::string> params;

				skipWhitespace();
				if (!match(')'))
				{
					while (true)
					{
						skipWhitespace();

						if (!std::isalpha(static_cast<unsigned char>(peek())))
						{
							m_Pos = startPos;
							return false;
						}

						std::string param;
						while (std::isalpha(static_cast<unsigned char>(peek())))
							param += advance();

						params.push_back(param);
						skipWhitespace();

						if (match(')'))
							break;

						if (!match(','))
						{
							m_Pos = startPos;
							return false;
						}
					}
				}

				skipWhitespace();
				if (!match('='))
				{
					m_Pos = startPos;
					return false;
				}

				auto body = ParseExpression();
				m_LastFunctionDef = FunctionDef{name, params, std::move(body)};
				return true;
			}


			// Helpers =========================================================================


			char peek() const
			{
				return m_Pos < m_Input.size() ? m_Input[m_Pos] : '\0';
			}

			char advance()
			{
				return m_Input[m_Pos++];
			}


			bool match(char c)
			{
				if (peek() == c)
				{
					advance();
					return true;
				}
				return false;
			}

			void skipWhitespace()
			{
				while (std::isspace(static_cast<unsigned char>(peek())))
					advance();
			}

			bool startsPrimary(char c) const
			{

				unsigned char uc = static_cast<unsigned char>(c);
				return std::isdigit(uc) || uc == '.' || std::isalpha(uc);
			}

			bool isFunctionName(const std::string &s) const
			{
				return STANDARD_FUNCTIONS::sf.contains(s) || m_FunctionEngine->HasFunction(s);;
			}
		};

	} // ================================ END DETAIL NAMESPACE ====================================




		// ======================================================================================
		// ==================================== PUBLIC API ======================================
		// ======================================================================================


		struct Vars {
			std::vector<std::pair<std::string, double>> list;

			Vars() = default;
			Vars(std::initializer_list<std::pair<std::string, double>> v)
				: list(v) {}
		};

		struct Funcs {
			std::vector<std::string> list;

			Funcs() = default;
			Funcs(std::initializer_list<std::string> d)
				: list(d) {}
		};


	/**
	 * @brief Construct and compile a mathematical expression.
	 *
	 * @param source Expression string to parse.
	 * @param func   Initializer list of function definitions available to the expression.
	 * Multiple functions are written as separate strings and separated by commas.
	 * @param vars   Initializer list of std::pair variable bindings, each variable is written separately as a std::pair.
	 *
	 * EXAMPLE USE: CompiledExpression expr("x^2 + y^2 + f(x)", Funcs{"f(x) = x^3"}, Vars{{"x", 2}, {"y", 4}})
	 *
	 *
	 * @note Function definitions are parsed eagerly; the expression itself
	 *       is parsed lazily on first evaluation. Order of funcs and vars
	 *       does not matter; one or neither may be supplied at object
	 *       instantiation.
	 */
	class CompiledExpression
	{
	private:
		std::string			m_Source;
		detail::ParseResult m_Result;
		detail::VarTable    m_Vars;
		bool				m_Parsed = false;
		std::shared_ptr<detail::FunctionEngine> m_FunctionEngine = std::make_shared<detail::FunctionEngine>();

	public:

		explicit CompiledExpression
		(
			const std::string& source,
			const Vars& vars = {},
			const Funcs& funcs = {}
		)
			: m_Source(RemoveNuls(source))
		{
			for (auto& [k, v] : vars.list)
				m_Vars[k] = v;
			for (auto& d : funcs.list)
			{

				std::string cleaned = RemoveNuls(d);

				// If a blob is passed, extract defs from it
				auto defsFound = ExtractDefs(cleaned);

				if (!defsFound.empty())
				{
					for (auto& one : defsFound)
						def(one);
				}
				else
				{
					// otherwise treat it as a single definition
					def(cleaned);
				}
			}

		}


		explicit CompiledExpression // Arg symmetry
		(
			const std::string& source,
			const Funcs& defs,
			const Vars& vars = {}
		)
		: CompiledExpression(source, vars, defs)
			{}

		/**
		 * @brief Define or redefine a user function.
		 *
		 * @param funcs  Function definitions available to the expression.
		 *
		 * @note Redefining a function invalidates any previously parsed expression.
		 * @throws std::runtime_error if the definition is malformed.
		 */
		void def(const std::string& definition)
		{
			size_t i = 0;
			while (i < definition.size())
			{
				// skip any separators
				while (i < definition.size() && definition[i] == '\0')
					++i;
				if (i >= definition.size()) break;

				size_t j = i;
				while (j < definition.size() && definition[j] != '\0')
					++j;

				std::string one = definition.substr(i, j - i);

				detail::Parser p(one, m_FunctionEngine);
				auto result = p.Parse();

				if (!std::holds_alternative<detail::FunctionDef>(result))
					throw std::runtime_error("Expected function definition");

				m_FunctionEngine->AddFunction(std::move(std::get<detail::FunctionDef>(result)));

				i = j + 1;
			}

			m_Parsed = false; // invalidate parse if it existed
		}

		/**
		 * @brief Define or redefine user function(s) (std::initializer_list overload).
		 *
		 * @param funcs  Function definition(s) available to the expression.
		 *
		 * @note Redefining a function invalidates any previously parsed expression.
		 * @throws std::runtime_error if the definition is malformed.
		 */
		void def(std::initializer_list<std::string_view> definitions)
		{
			for (std::string_view d : definitions)
			{
				def(std::string(d));
			}
		}


		/**
		 * @brief Evaluate the compiled expression.
		 *
		 * @return The numeric value (double) of the expression.
		 * @throws std::runtime_error if the expression is not scalar or evaluation fails.
		 */
		double value()
		{
			if (!m_Parsed)
			{
				detail::Parser p(m_Source, m_FunctionEngine);
				m_Result = p.Parse();

				if (!std::holds_alternative<detail::ExprPtr>(m_Result))
					throw std::runtime_error("Expression is not scalar");

				m_Parsed = true;
			}

			return std::get<detail::ExprPtr>(m_Result)->Eval(m_Vars);
		}

		/**
		 * @brief Set or update the value of a variable.
		 *
		 * @note Does not trigger reparsing; the new value is used on the next
		 *       evaluation.
		 */
		void set_vars(const std::string& name, double value) { m_Vars[name] = value; }

		/**
		 * @brief Set or update the value of variable(s) (std::initializer_list overload.
		 *
		 * @note Does not trigger reparsing; the new value is used on the next
		 *       evaluation.
		 */
		void set_vars(std::initializer_list<std::pair<std::string, double>> vars)
		{
			for (const auto& [name, value] : vars)
				m_Vars[name] = value;
		}


	private:
		// Removes "\0" at beginning and end; issue in testing; fix could be more robust, fine for now
		static std::string RemoveNuls(std::string s)
		{
			s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
			return s;
		}

		static std::string Trim(std::string s)
		{
			auto notSpace = [](unsigned char c){ return !std::isspace(c); };
			while (!s.empty() && !notSpace((unsigned char)s.front())) s.erase(s.begin());
			while (!s.empty() && !notSpace((unsigned char)s.back()))  s.pop_back();
			return s;
		}

		static bool LooksLikeFunctionDef(const std::string& s)
		{
			// very simple filter: name '(' ... ')' '=' must exist in order
			auto lp = s.find('(');
			auto rp = s.find(')');
			auto eq = s.find('=');
			return lp != std::string::npos &&
				   rp != std::string::npos &&
				   eq != std::string::npos &&
				   lp < rp && rp < eq &&
				   lp > 0; // has a name
		}

		static std::vector<std::string> ExtractDefs(const std::string& blob)
		{
			std::vector<std::string> out;

			// split on '\0' first
			size_t i = 0;
			while (i < blob.size())
			{
				size_t j = i;
				while (j < blob.size() && blob[j] != '\0') ++j;

				std::string tok = Trim(blob.substr(i, j - i));

				// strip "Def:" prefix if present
				if (tok.rfind("Def:", 0) == 0)
					tok = Trim(tok.substr(4));

				// cut off at " (expected"
				if (auto k = tok.find(" (expected"); k != std::string::npos)
					tok = Trim(tok.substr(0, k));

				if (LooksLikeFunctionDef(tok))
					out.push_back(tok);

				i = j + 1;
			}

			return out;
		}
	}; // End class

	// ================== RUNTIME INPUT HELPERS ================
	static std::vector<std::string> ParseVars(const char* buf)
	{
		std::vector<std::string> out;
		if (!buf) return out;

		std::string s(buf);

		size_t i = 0;
		while (i < s.size())
		{
			size_t j = s.find(',', i);
			if (j == std::string::npos) j = s.size();

			std::string v = s.substr(i, j - i);

			// trim spaces
			auto first = v.find_first_not_of(" \t");
			auto last  = v.find_last_not_of(" \t");
			if (first != std::string::npos)
				v = v.substr(first, last - first + 1);
			else
				v.clear();

			if (!v.empty())
				out.push_back(v);

			i = j + 1;
		}
		return out;
	}

	static std::vector<double> ParseValues(const char* buf)
	{
		std::vector<double> out;
		if (!buf) return out;

		std::string s(buf);

		size_t i = 0;
		while (i < s.size())
		{
			size_t j = s.find(',', i);
			if (j == std::string::npos) j = s.size();

			std::string v = s.substr(i, j - i);

			// trim spaces
			auto first = v.find_first_not_of(" \t");
			auto last  = v.find_last_not_of(" \t");
			if (first != std::string::npos)
				v = v.substr(first, last - first + 1);
			else
				v.clear();

			if (!v.empty())
			{
				try
				{
					double d = std::stod(v);

					if (std::isfinite(d))
						out.push_back(d);
					// else: silently ignore non-finite
				}
				catch (...)
				{
					std::cerr << "[Math Parser] Invalid Numeric Input" << std::endl;
					// swallow invalid numeric input
					// do NOT throw
				}
			}

			i = j + 1;
		}
		return out;
	}

	/* USE:

		auto vars   = ParseVars(varBuffer);      // "x,y"
		auto values = ParseValues(valueBuffer);  // "4,3"

		if (vars.size() != values.size())
		{
			LOG_ERROR("Variable/value count mismatch");
		}
		else
		{
			Vars bindings;
			for (size_t i = 0; i < vars.size(); ++i)
				bindings.list.emplace_back(vars[i], values[i]);

			CompiledExpression expr(expression, bindings);
			LOG(expr.value());
		}
	*/


} // ============================= END MATHPARSER NAMESPACE ================================