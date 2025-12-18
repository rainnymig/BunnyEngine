struct Complex
{
    float real;
    float imaginary;
};

Complex conjugate(Complex c)
{
    return Complex(c.real, -c.imaginary);
}

Complex add(Complex c1, Complex c2)
{
    return Complex(c1.real + c2.real, c1.imaginary + c2.imaginary);
}

Complex subtract(Complex c1, Complex c2)
{
    return Complex(c1.real - c2.real, c1.imaginary - c2.imaginary);
}

Complex multiply(Complex c1, Complex c2)
{
    return Complex(c1.real * c2.real - c1.imaginary * c2.imaginary, c1.imaginary * c2.real + c1.real * c2.imaginary);
}

Complex multiply(Complex c, float r)
{
    return Complex(c.real * r, c.imaginary * r);
}

//  e^ix
Complex expi(float x)
{
    //  Euler formula
    return Complex(cos(x), sin(x));
}

//  e^ic
Complex expi(Complex c)
{
    return multiply(expi(c.real), exp(-c.imaginary));
}