#include <cassert>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>

using varmap = std::map<std::string, double>;

class Token
{
public:
    enum Type
    {
	Varname,
	Number,
	Plus,
	Minus,
	Mult,
	Divide,
	LParen,
	RParen,
	Equal,
	SemiColon,
	EndOfFile,
	Undefined
    };
    Type        type;
    std::string value;
    Token(const std::string& v, Type t) : type(t), value(v) {}
    Token(Type t) : type(t) {}
    Token() : type(Undefined) {}
};

class ConstExpr;
class ConstUnaryExpr;

class Value
{
public:
    enum Type
    {
	Constant,
	Variable,
	Expr,
	UnaryExpr,
	Unknown
    };

    Value(double d) : type(Constant), value(d) {}
    Value(const std::string& s) : type(Variable), varname(s) {}
    Value(ConstExpr* e) : type(Expr), expr(e) {}
    Value(ConstUnaryExpr* u) : type(UnaryExpr), unary(u) {}
    Value() : type(Unknown) {}

    ~Value() {}

    double operator()() const;

private:
    Type type;
    std::string varname;
    double      value;
    ConstExpr*  expr;
    ConstUnaryExpr* unary;
};

class ConstUnaryExpr
{
public:
    ConstUnaryExpr(Token::Type t, const Value r) : op(t), rhs(r) {}

    double Evaluate() const;

private:
    Token::Type op;
    const Value rhs;
};

class ConstExpr
{
public:
    ConstExpr(const Value l, Token::Type t, const Value r) : lhs(l), op(t), rhs(r) {}

    double Evaluate() const;

private:
    const Value  lhs;
    Token::Type  op;
    const Value  rhs;
};

varmap vars;
bool   verbose = false;
Token  curToken;
bool   curValid = false;

std::tuple<bool, double> FindVar(const std::string& name)
{
    varmap::iterator it = vars.find(name);
    if (it != vars.end())
	return { true, it->second };
    std::cout << "Invalid variable " << name << std::endl;
    return { false, 0.0 };
}

double ConstUnaryExpr::Evaluate() const
{
    switch (op)
    {
    case Token::Plus:
	return rhs();

    case Token::Minus:
	return -rhs();
    default:
	std::cout << "Unknown operation: " << op << std::endl;
	return 0;
    }
}

double ConstExpr::Evaluate() const
{
    switch (op)
    {
    case Token::Plus:
	return lhs() + rhs();
    case Token::Minus:
	return lhs() - rhs();
    default:
	std::cout << "Unknown operation: " << op << std::endl;
	return 0;
    }
}

double Value::operator()() const
{
    switch (type)
    {
    case Constant:
	return value;

    case Variable:
    {
	auto [found, value] = FindVar(varname);
	if (found)
	    return value;
	return 0.0;
    }
    case Expr:
    {
	return expr->Evaluate();
    }
    case UnaryExpr:
    {
	return unary->Evaluate();
    }
    case Unknown:
	assert(0 && "Uninitielized value");
	return 0.0;
    }
}

unsigned TokenPrio(Token::Type t)
{
    switch (t)
    {
    case Token::Mult:
    case Token::Divide:
	return 2;
    case Token::Plus:
    case Token::Minus:
	return 1;
    default:
	return 0;
    }
}

Token GetNextToken()
{
    std::string v;
    while (1)
    {
	int ch = std::cin.get();
	if (ch == EOF)
	{
	    return Token::EndOfFile;
	}
	if (isspace(ch))
	{
	    continue;
	}
	if (isalpha(ch))
	{
	    while (isalnum(ch))
	    {
		v += ch;
		ch = std::cin.get();
	    }
	    std::cin.putback(ch);
	    return Token(v, Token::Varname);
	}
	if (isdigit(ch))
	{
	    while (isdigit(ch))
	    {
		v += ch;
		ch = std::cin.get();
	    }
	    std::cin.putback(ch);
	    return Token(v, Token::Number);
	}
	switch (ch)
	{
	case '+':
	    return Token(Token::Plus);
	case '-':
	    return Token(Token::Minus);
	case '*':
	    return Token(Token::Mult);
	case '/':
	    return Token(Token::Divide);
	case '=':
	    return Token(Token::Equal);
	case '(':
	    return Token(Token::LParen);
	case ')':
	    return Token(Token::RParen);
	case ';':
	    return Token(Token::SemiColon);
	default:
	    std::cout << "Uh? found character '" << ch << "' which doesn't seem to be useful here"
	              << std::endl;
	    break;
	}
    }
}

Token GetToken()
{
    if (!curValid)
    {
	curToken = GetNextToken();
	curValid = true;
    }
    return curToken;
}

void NextToken()
{
    curValid = false;
}

double ToDouble(const std::string& val)
{
    double       d;
    std::stringstream ss(val);
    if (ss >> d)
    {
	return d;
    }
    std::cout << "Invalid number, replacing with -1" << std::endl;
    return -1.0;
}

bool Expect(Token::Type ty, Token& t)
{
    t = GetToken();
    NextToken();
    if (t.type != ty && t.type != Token::EndOfFile)
    {
	std::cout << "Invalid token, expected: " << ty << " got " << t.type << std::endl;
	return false;
    }
    return true;
}

Value ParseValue();

Value ParseSimpleExpr()
{
    Token t = GetToken();
    switch (t.type)
    {
    case Token::Number:
	NextToken();
	return Value(ToDouble(t.value));

    case Token::Varname:
	NextToken();
	return Value(t.value);

    case Token::Plus:
    case Token::Minus:
	NextToken();
	return Value(new ConstUnaryExpr(t.type, ParseValue()));

    case Token::EndOfFile:
	break;

    default:
	std::cout << "Unknown value" << std::endl;
	break;
    }
    return Value(0.0);
}

Value ParseValue()
{
    for (;;)
    {

	Value lhs = ParseSimpleExpr();

	Token t = GetToken();

	if (verbose)
	{
	    std::cout << "Token: " << t.type << " value:" << t.value << std::endl;
	}
	switch (t.type)
	{
	case Token::Plus:
	case Token::Minus:
	    NextToken();
	    return Value(new ConstExpr(lhs, t.type, ParseValue()));

	case Token::Equal:
	{
	    std::cout << "Error: Unexpected '='" << std::endl;
	    NextToken();
	    break;
	}

	case Token::EndOfFile:
	    return Value(-1);

	case Token::SemiColon:
	    return lhs;

	default:
	    std::cout << "Error, unknown token" << std::endl;
	    NextToken();
	    break;
	}
    }
}

void Parse()
{
    Token v;
    do
    {
	if (Expect(Token::Varname, v))
	{
	    Token e;
	    if (Expect(Token::Equal, e))
	    {
		Value val = ParseValue();
		NextToken();
		vars[v.value] = val();
		std::cout << "val=" << val() << std::endl;
	    }
	}
    } while (v.type != Token::EndOfFile);
}

void Usage(const std::string& msg, const std::string& option = "")
{
    if (msg != "")
    {
	std::cerr << msg;
	if (option != "")
	{
	    std::cerr << ":" << option;
	}
	std::cerr << "\n\n";
    }
    std::cerr << "Options available:\n";
    std::cerr << "-v     Enable verbose mode" << std::endl;
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
	if (argv[i][0] != '-')
	{
	    Usage("Not an option", argv[i]);
	    exit(1);
	}
	const std::string a = argv[i];
	if (a == "-v")
	{
	    verbose = true;
	}
	else
	    Usage("Invalid option", a);
    }

    Parse();
}
