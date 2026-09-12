#include<bits/stdc++.h>
using namespace std;
/*
ワーシャル・フロイド法
すべての2頂点間の最短経路を求める

計算量：O(|V|^3)

負の辺があってもOK
負の閉路がある場合、d[i][j]が負である頂点が存在
*/

#define MAX_V 10000000 //頂点数の最大値,10^7
#define INF 2000000000 //2*10^9

int d[MAX_V][MAX_V];//d[u][v]は辺e=(u,v)のコスト（存在しない場合はINF,d[i][i]=0）
int V;//辺の数

void warshall_floyd(){
    for(int k=0;k<V;k++){
        for(int i=0;i<V;i++){
            for(int j=0;j<V;j++){
                d[i][j]=min(d[i][j],d[i][k]+d[k][j]);
            }
        }
    }
}
