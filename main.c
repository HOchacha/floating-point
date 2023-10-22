
#include <stdio.h>
#include <stdint.h>
#include <string.h>


typedef struct _sign{
    uint32_t val : 1;
}signVal;

typedef struct _exp{
    uint32_t val : 8;
}expVal;
typedef struct _fracVal{
    uint32_t val : 24;
}fracVal;

typedef union _floatData{
    float a;
    struct {
        uint32_t sign : 1;
        uint32_t exp  : 8;
        uint32_t frac : 23;
    }part;
};
//1.0 * xxxx 형태 표현 시, 정규화 정수부도 고려 필요 -> 24비트로 배열 선언 필요. 하지만, 결국에는 정수부에서 발생하는 오버플로우를 다루기 위해서는 어쩔 수 없이 Frac을 25비트로 선언하는 것이 맞는 것 같다.
uint32_t addition(uint32_t operand1 , uint32_t operand2){
    printf("Input : %d %d\n", operand1, operand2);
    printf("Input : %08X %08X\n", operand1, operand2);
    //extract sign, exp, frac
    signVal op1Sign;
    signVal op2Sign;

    expVal op1Exp;
    expVal op2Exp;

    fracVal op1Frac;
    fracVal op2Frac;

//1000 0000 0000 0000  0000 0000 0000 0000
    op1Sign.val = (operand1 & 0x80000000) >> 31;
    op2Sign.val = (operand2 & 0x80000000) >> 31;
    printf("Sign : %d %d\n", op1Sign.val, op2Sign.val);
//0111 1111 1000 0000  0000 0000 0000 0000
    op1Exp.val = (((operand1 & 0x7F800000) >> 23) & 0xFF);
    op2Exp.val = (((operand2 & 0x7F800000) >> 23) & 0xFF);
    printf("Exp operand1 : 0x%08X :: %d\n", op1Exp.val, op1Exp.val);
    printf("Exp operand2 : 0x%08X :: %d\n", op2Exp.val, op2Exp.val);
//0000 0000 0111 1111  1111 1111 1111 1111
    op1Frac.val= (operand1 & 0x7FFFFF);
    op2Frac.val = operand2 & 0x7FFFFF;
    op1Frac.val |= 0x800000;
    op2Frac.val |= 0x800000;
    printf("init operand1 Frac : %08X\n", op1Frac.val);
    printf("init operand2 Frac : %08X\n", op2Frac.val);
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

    printf("shifted operand1 Frac : %08X\n", op1Frac.val);
    printf("shifted operand2 Frac : %08X\n", op2Frac.val);

//{end}
    expVal tempExp;
    signVal tempSign;
    fracVal tempFrac;

    int flag = 0;
    //덧셈상황
    if(op1Sign.val == op2Sign.val ){
        tempFrac.val = (op1Frac.val )+ (op2Frac.val);

        uint32_t carry = (((op1Frac.val & 0x400000) >> 22) && ((op2Frac.val & 0x400000) >> 22));
        if(((op1Frac.val >> 23) && (op2Frac.val >> 23)) || (((op1Frac.val & 0x800000) >> 23 ^ (op2Frac.val & 0x800000) >> 23) && carry)){
            printf("Flagged!!\n");
            flag = 1;
        }
        printf("TempFrac :: %08X\n", tempFrac.val);
        tempSign.val = op1Sign.val;
    }
    //뺄셈 상황
    else{
        if(op1Frac.val > op2Frac.val) {
            tempFrac.val = op1Frac.val - op2Frac.val;
            tempSign.val = op1Sign.val;
        }
        else{
            tempFrac.val = op2Frac.val - op1Frac.val;
            tempSign.val = op2Sign.val;
        }
        //캐리 발생 감지
        printf("Frac's 24th bit : %d\n", (tempFrac.val & 0x800000));
        if(((tempFrac.val & 0x800000) >> 23) == 0) {
            printf("Flagged!!\n");
            flag = -1;
        }
        printf("TempFrac :: %08X\n", tempFrac.val);
    }

    tempExp.val = op1Exp.val > op2Exp.val ? op1Exp.val : op2Exp.val;
    //오버플로우 언더플로우 핸들링
    if(flag == 1){
        tempExp.val++;
        tempFrac.val >>= 1;
    }
    else if(flag == -1){
        while((tempFrac.val & 0x800000) == 0){
            tempExp.val--;
            tempFrac.val <<= 1;
        }
    }
    printf("Handled temp Frac : %08X", tempFrac.val);
    uint32_t result = 0;
    printf("%08X   %08X   %08X\n", tempSign.val, tempExp.val, tempFrac.val );
    printf("%d   %d   %d\n", tempSign.val, tempExp.val, tempFrac.val );
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
    float res = uint32_to_float(addition(float_to_uint32(6.625), float_to_uint32(6.625) ));
    printf("%f %f\n\n\n\n", 6.625 + 6.625, res);

    printf("0x%08X 0x%08X\n\n\n\n\n", float_to_uint32(6.625 + 6.625), addition(float_to_uint32(6.625), float_to_uint32(6.625) ));


    printf("0x%08X 0x%08X\n\n\n\n\n", float_to_uint32(12.25 + 6.625), addition(float_to_uint32(12.25), float_to_uint32(6.625) ));
    printf("%f %f\n\n\n\n\n", 12.25 + 6.625, uint32_to_float(addition(float_to_uint32(12.25), float_to_uint32(6.625))));

    printf("0x%08X 0x%08X\n\n\n\n\n", float_to_uint32(12.25 - 6.625), addition(float_to_uint32(12.25), float_to_uint32(-6.625) ));
    printf("0x%08X 0x%08X\n\n\n\n\n", float_to_uint32(12.136 - 16.25), addition(float_to_uint32(12.136), float_to_uint32(-16.25) ));
}


