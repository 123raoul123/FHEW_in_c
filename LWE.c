#include "FHEW.h"
#include "params.h"
#include "stdbool.h"
#include <stdlib.h>

/*************************************************************************
*                                                                        *
*                           KEYGENERATION                                *
*                                                                        *
*************************************************************************/

void LWEKeyGen(SecretKey sk) {
  KeyGenRestart:;
  int s=0, ss=0;
  printf("Restart\n");
  for (int i = 0; i < n; ++i) {
    sk[i] = Sample(Chi_Binary);
    s+= sk[i];
    ss+= abs(sk[i]);
  }
  if (abs(s)>5) 
    goto KeyGenRestart;
  if (abs(ss - n/2)>5) 
    goto KeyGenRestart;
  printf("Finished LWEKEYGEN\n");
}

void KeyGenN(SecretKeyN FHEWsk) {
	printf("\n*****Starting KeygenN*****\n");
  for(int i =0;i < N; ++i){
    FHEWsk[i] = Sample(Chi1);
    // printf("FHEWsk[%d] = %d\n",i,FHEWsk[i]);
  }
  
  printf("\n*****Finished KeygenN*****\n");

}

//GENERATE SwitchingKey //SwitchingKey => CipherTextQ[1024][25][7] //CipherTextQ = {ZmodQ a[n]; ZmodQ b;} => ZmodQ = int32_t and n = 500 
void SwitchingKeyGen(SwitchingKey res,SecretKey new_sk,SecretKeyN old_sk) {
  printf("\n*****Starting SwitchingKeyGen*****\n");
  for (int i = 0; i < N; ++i){ 
    for (int j = 0; j < KS_base; ++j){
      for (int k = 0; k < KS_exp; ++k) 
      {
        CipherTextQ ct;    
        ct.b = -old_sk[i]*j*KS_table[k] + Sample(Chi2);
        for (int l = 0; l < n; ++l) 
        {
          ct.a[l] = rand(); //I need to get this positive
          ct.b += ct.a[l] * new_sk[l];
        }
        res[i][j][k] = ct;
      }
    }
    printf("i is now : %d\n",i);
  }
  printf("\n*****Finished SwitchingKeyGen*****\n");
}

/*************************************************************************
*                                                                        *
*                           DECRYPT ENCRYPT                              *
*                                                                        *
*************************************************************************/

void Encrypt(CipherText* ct, SecretKey sk, int m) {
    ct->b = (m % 4) * q / 4;//can you just do m * q / 4 because m is 0 or 1 this is always the same mod 4
    ct->b += Sample(Chi3);
    for (int i = 0; i < n; ++i)	
    {
      ct->a[i] = rand() % q;
      ct->b = (ct->b + ct->a[i] * sk[i]) % q;
    }
}

int Decrypt(SecretKey sk, CipherText* ct) {
    int r = ct->b;
    for (int i = 0; i < n; ++i) 
      r -= ct->a[i] * sk[i];
    r = ((r % q) + q + q/8) % q;
    return 4 *r/q;    
}

// void DecryptDetail(const SecretKey sk, const CipherText& ct) {

// }

/*************************************************************************
*                                                                        *
*                           OTHER STUFF                                  *
*                                                                        *
*************************************************************************/
    
int round_qQ(ZmodQ var) {
   return floor(.5 + (double) var * (double) q / (double) Q) + q % q;
}
  
void ModSwitch(CipherText* ct, CipherTextQ* c) {
  for (int i = 0; i < n; ++i) 
    ct->a[i] = round_qQ(c->a[i]);  
  ct->b = round_qQ(c->b);
}
  
void KeySwitch(CipherTextQ* res, SwitchingKey Key, CipherTextQN* ct) {
  //SwitchingKey => CipherTextQ[1024][25][7]
  for (int k = 0; k < n; ++k) 
    res->a[k] = 0;
    
  res->b = ct->b;
  for (int i = 0; i < N; ++i) 
  {
    uZmodQ a = -ct->a[i];
    for (int j = 0; j < KS_exp; ++j, a /= KS_base) 
    {
      uZmodQ a0 = a % KS_base;
      for (int k = 0; k < n; ++k)
        res->a[k] -= (Key[i][a0][j]).a[k];
      
      res->b -= (Key[i][a0][j]).b;
    }
  } 
}
  


