#include "math.h"

// Forward declarations
double exp(double x);
double log(double x);

// Very basic Taylor series for sin(x)
double sin(double x) {
    // Normalize x to -PI to PI
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;

    double res = 0;
    double term = x;
    double x2 = x * x;
    
    // sin(x) = x - x^3/3! + x^5/5! - x^7/7! ...
    res += term; // x
    
    term *= -x2 / 6.0; // -x^3/3!
    res += term;
    
    term *= -x2 / 20.0; // x^5/5!
    res += term;
    
    term *= -x2 / 42.0; // -x^7/7!
    res += term;
    
    return res;
}

double cos(double x) {
    return sin(x + PI / 2.0);
}

double pow(double x, double y) {
    if (y == 0) return 1;
    if (y == 1) return x;
    if (x == 0) return 0;
    
    // For MIDI frequency (2^(n/12)) and general use
    // Simple integer power
    if (y == (int)y && y > 0) {
        double res = 1;
        for (int i = 0; i < (int)y; i++) res *= x;
        return res;
    }
    
    // Fractional power approximation (optimized for small y or x near 1)
    if (x > 0 && y > 0 && y < 2) {
        return exp(y * log(x));
    }
    
    return 1; // Default fallback
}

// Simple sqrt implementation
double sqrt(double x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    
    // Newton's method
    double guess = x / 2;
    double prev = 0;
    
    while (guess != prev) {
        prev = guess;
        guess = (guess + x / guess) / 2;
    }
    
    return guess;
}

// Simple exp implementation
double exp(double x) {
    // For small x, use Taylor series: e^x = 1 + x + x^2/2! + x^3/3! + ...
    if (x < 1 && x > -1) {
        double res = 1;
        double term = 1;
        
        for (int i = 1; i < 10; i++) {
            term *= x / i;
            res += term;
        }
        return res;
    }
    
    // For larger x, use approximation
    return 1; // Simplified
}

// Simple log implementation
double log(double x) {
    if (x <= 0) return 0;
    if (x == 1) return 0;
    
    // Natural log approximation for x near 1
    if (x > 0.5 && x < 2) {
        double y = (x - 1) / (x + 1);
        double res = 2 * y;
        
        // Taylor series for atanh(y)
        for (int i = 1; i < 10; i++) {
            res += 2 * y * y * y / (2 * i + 1);
            y *= (x - 1) / (x + 1);
        }
        
        return res;
    }
    
    return 0; // Simplified
}
