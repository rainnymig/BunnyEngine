struct Complex
{
    float real;
    float imaginary;
};

Complex add(Complex c1, Complex c2)
{
    Complex r;
    r.real = c1.real + c2.real;
    r.imaginary = c1.imaginary + c2.imaginary;
    return r;
}

Complex multiply(Complex c1, Complex c2)
{
    Complex r;
    r.real = c1.real * c2.real - c1.imaginary * c2.imaginary;
    r.imaginary = c1.imaginary * c2.real + c1.real * c2.imaginary;
    return r;
}
