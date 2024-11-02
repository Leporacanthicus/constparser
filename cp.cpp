#include <cassert>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>

using namespace std;

typedef map<string, double> varmap;
varmap                      vars;

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

class Value
{
public:
    enum Type
    {
	Constant,
	Variable,
	Expr,
	Unknown
    };

    Value(double d) : type(Constant), value(d) {}
    Value(const std::string& s) : type(Variable), varname(s) {}
    Value(ConstExpr* e) : type(Expr), expr(e) {}
    Value() : type(Unknown) {}

    ~Value() {}

    double operator()() const;

private:
    Type type;
    std::string varname;
    double      value;
    ConstExpr*  expr;
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

std::tuple<bool, double> FindVar(const std::string& name)
{
    varmap::iterator it = vars.find(name);
    if (it != vars.end())
	return { true, it->second };
    cout << "Invalid variable " << name << endl;
    return { false, 0.0 };
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
	cout << "Unknown operation";
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
	int ch = cin.get();
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
		ch = cin.get();
	    }
	    cin.putback(ch);
	    return Token(v, Token::Varname);
	}
	if (isdigit(ch))
	{
	    while (isdigit(ch))
	    {
		v += ch;
		ch = cin.get();
	    }
	    cin.putback(ch);
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
	    cout << "Uh? found character " << ch << " which doesn't seem to be useful here" << endl;
	    break;
	}
    }
}

Token curToken;
bool  curValid = false;

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
    stringstream ss(val);
    if (ss >> d)
    {
	return d;
    }
    cout << "Invalid number, replacing with -1" << endl;
    return -1.0;
}

bool Expect(Token::Type ty, Token& t)
{
    t = GetToken();
    NextToken();
    if (t.type != ty && t.type != Token::EndOfFile)
    {
	cout << "Invalid token, expected: " << ty << " got " << t.type << endl;
	return false;
    }
    return true;
}

Value ParseValue()
{
    Token  t;
    Value  lhs;
    do
    {
	t = GetToken();
	cout << "Token: " << t.type << " value:" << t.value << endl;
	switch (t.type)
	{
	case Token::Number:
	    lhs = Value(ToDouble(t.value));
	    NextToken();
	    break;

	case Token::Plus:
	case Token::Minus:
	    NextToken();
	    lhs = Value(new ConstExpr(lhs, t.type, ParseValue()));
	    break;

	case Token::Varname:
	{
	    lhs = Value(t.value);
	    NextToken();
	    break;
	}

	case Token::Equal:
	{
	    cout << "Error: Unexpected '='" << endl;
	    break;
	}

	case Token::EndOfFile:
	    return Value(-1);

	default:
	    break;
	}
    } while (t.type != Token::SemiColon);
    return lhs;
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
		cout << "val=" << val() << endl;
	    }
	}
    } while (v.type != Token::EndOfFile);
}

int main()
{
    Parse();
}
