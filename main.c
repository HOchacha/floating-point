#include <stdio.h>
#include <stdint.h>
#include <string.h>




//if you want to print only result, please make the MACRO comment.
#define DEBUG

typedef struct _sign{
    uint32_t val : 1;
}signVal;

typedef struct _exp{
    uint32_t val : 8;
}expVal;
typedef struct _fracVal{
    uint32_t val : 24;
}fracVal;


// **For warning!!
// this code was tested only normalized condition.
// any further condition will be added, such as de-normalization, inf, NaN.
// Also, the actual arithmetic is implemented by the default addition operator.
// So, if there is any condition of not using + operator, you may have to reconfigure the arithmetic calculation by "AND OR NOT XOR"
// it will be horrible thing to do on test time. But it might be worth it to implement by that way.

uint32_t float_Arithmetic(uint32_t operand1 , uint32_t operand2){
#ifdef DEBUG
    printf("Input : %d %d\n", operand1, operand2);
    printf("Input : %08X %08X\n", operand1, operand2);
#endif
//{begin} extract sign, exp, frac
    signVal op1Sign;
    signVal op2Sign;

    expVal op1Exp;
    expVal op2Exp;

    fracVal op1Frac;
    fracVal op2Frac;

    //1000 0000 0000 0000  0000 0000 0000 0000
    op1Sign.val = (operand1 & 0x80000000) >> 31;
    op2Sign.val = (operand2 & 0x80000000) >> 31;

    //0111 1111 1000 0000  0000 0000 0000 0000
    op1Exp.val = (((operand1 & 0x7F800000) >> 23) & 0xFF);
    op2Exp.val = (((operand2 & 0x7F800000) >> 23) & 0xFF);

    //0000 0000 0111 1111  1111 1111 1111 1111
    op1Frac.val= (operand1 & 0x7FFFFF);
    op2Frac.val = operand2 & 0x7FFFFF;
    op1Frac.val |= 0x800000;
    op2Frac.val |= 0x800000;
#ifdef DEBUG
    printf("Sign : %d %d\n", op1Sign.val, op2Sign.val);
    printf("Exp operand1 : 0x%08X :: %d\n", op1Exp.val, op1Exp.val);
    printf("Exp operand2 : 0x%08X :: %d\n", op2Exp.val, op2Exp.val);
    printf("init operand1 Frac : %08X\n", op1Frac.val);
    printf("init operand2 Frac : %08X\n", op2Frac.val);
#endif
//{end} extract sign, exp, frac

//{begin} major에 맞춰서 minor 비트 시프트
    uint8_t temp = op1Exp.val > op2Exp.val ? op1Exp.val - op2Exp.val : op2Exp.val - op1Exp.val;
    if(op1Exp.val < op2Exp.val){
        for(int i = 0; i < temp; i++) {
            op1Frac.val = op1Frac.val >> 1;
        }
        op1Exp.val += temp;
    }
    else if (op1Exp.val > op2Exp.val){
        for(int i = 0; i < temp; i++) {
            op2Frac.val = op2Frac.val >> 1;
        }
        op2Exp.val += temp;
    }
#ifdef DEBUG
    printf("shifted operand1 Frac : %08X\n", op1Frac.val);
    printf("shifted operand2 Frac : %08X\n", op2Frac.val);
#endif

//{end} major에 맞춰서 minor 비트 시프트
//{begin} arithmetic (float_Arithmetic, subtraction)
    expVal tempExp;
    signVal tempSign;
    fracVal tempFrac;

    int flag = 0;
    //float_Arithmetic, this is for same sign bits
    if(op1Sign.val == op2Sign.val ){
        tempFrac.val = (op1Frac.val )+ (op2Frac.val);


        //we need to get the carry to address overflow.
        //the idea was from the 4 bit adder (array of the Full adder) by Logic circuit.
        //the carry will be occured when either both operand's 24th bit are same or the sum of them and 23rd bit's carry are same.
        //flagged, this may occur the bit shifting to make sure the normalization/
        uint32_t carry = (((op1Frac.val & 0x400000) >> 22) && ((op2Frac.val & 0x400000) >> 22));
        if(((op1Frac.val >> 23) && (op2Frac.val >> 23)) || (( ((op1Frac.val & 0x800000) >> 23) ^ (op2Frac.val & 0x800000) >> 23) && carry)){
#ifdef DEBUG
            printf("Flagged!!\n");
#endif
            flag = 1;
        }
#ifdef DEBUG
        printf("TempFrac :: %08X\n", tempFrac.val);
#endif
        tempSign.val = op1Sign.val;
    }
    //subtraction, this is for different sign bits.
    //when the subtraction occur, the sign bit follow that of the major.
    else{
        //major subtracted by minor.
        if(op1Frac.val > op2Frac.val) {
            tempFrac.val = op1Frac.val - op2Frac.val;
            tempSign.val = op1Sign.val;
        }
        else{
            tempFrac.val = op2Frac.val - op1Frac.val;
            tempSign.val = op2Sign.val;
        }

        //this is the way to detect the Underflow, which the result of 24th bit is zero.
        //the flag will shift the bits to make sure the normalization.
#ifdef DEBUG
        printf("Frac's 24th bit : %d\n", (tempFrac.val & 0x800000));
#endif
        if(((tempFrac.val & 0x800000) >> 23) == 0) {
#ifdef DEBUG
            printf("Flagged!!\n");
#endif
            flag = -1;
        }
#ifdef DEBUG
        printf("TempFrac :: %08X\n", tempFrac.val);
#endif
    }

    tempExp.val = op1Exp.val > op2Exp.val ? op1Exp.val : op2Exp.val;

    //this is the handler of Overflow and Underflow.
    if(flag == 1){
        //when the overflow occured, this just make a carry, which means we just shift once in this situation.
        tempExp.val++;
        tempFrac.val >>= 1;
    }
    else if(flag == -1){
        //Also, Underflow occured, we have to make sure the form follows the normalization.
        //So, we have to pull up the bits until the 24th bit is 1. if not, this form will break the rules of the IEEE floating point representation.
        while((tempFrac.val & 0x800000) == 0){
            tempExp.val--;
            tempFrac.val <<= 1;
        }
    }

    //we have to pack the seperated data.
#ifdef DEBUG
    printf("Handled temp Frac : %08X", tempFrac.val);
    printf("%08X   %08X   %08X\n", tempSign.val, tempExp.val, tempFrac.val );
    printf("%d   %d   %d\n", tempSign.val, tempExp.val, tempFrac.val );
#endif
    uint32_t result = 0;


    //you have to know that we do not notate the 24th bit.
    //we use for 24th to represent and manipulate the normalized situation.
    result = ((uint32_t)tempFrac.val & 0x7FFFFF)| (((uint32_t)tempExp.val& 0xFF) << 23) | (((uint32_t)tempSign.val ) <<31);
    return result;
}
uint32_t float_to_uint32(float f) {
    uint32_t result;
    memcpy(&result, &f, sizeof(f));
    return result;
}

float uint32_to_float(uint32_t u) {
    float result;
    memcpy(&result, &u, sizeof(u));
    return result;
}



int main(){
    float res = uint32_to_float(float_Arithmetic(float_to_uint32(6.625), float_to_uint32(6.625)));
    printf("%f %f\n\n\n", 6.625 + 6.625, res);

    printf("0x%08X 0x%08X\n\n\n", float_to_uint32(6.625 + 6.625),
           float_Arithmetic(float_to_uint32(6.625), float_to_uint32(6.625)));

    printf("0x%08X 0x%08X\n\n\n", float_to_uint32(12.25 + 6.625),
           float_Arithmetic(float_to_uint32(12.25), float_to_uint32(6.625)));
    printf("%f %f\n\n\n", 12.25 + 6.625, uint32_to_float(float_Arithmetic(float_to_uint32(12.25), float_to_uint32(6.625))));

    printf("0x%08X 0x%08X\n\n\n", float_to_uint32(12.25 - 6.625),
           float_Arithmetic(float_to_uint32(12.25), float_to_uint32(-6.625)));
    printf("0x%08X 0x%08X\n\n\n", float_to_uint32(12.136 - 16.25),
           float_Arithmetic(float_to_uint32(12.136), float_to_uint32(-16.25)));
}


