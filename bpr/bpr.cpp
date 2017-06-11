/* ×¢ÊÍ¼ûbpr-transr.cpp */
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include <time.h>
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <random>

using namespace std;

#define pi 3.1415926535897932384626433832795

int dim=100;//15 100 10
double learning_rate=0.01;//0.03  0.03 0.01
double lamda_bias=1;//0.1  0.1 0.1
double lamda_u=0.01;//0.10  0.105 0.1075
double lamda_i=0.01;//0.10  0.105 0.1075
double lamda_j=0.01;//0.10  0.105 0.1075
int topk=100;
int maxiter=1000;
double split_into_test_ratio=0.3;

char* get_time(char* tmp);
double vdot(double* user_vector,double* movie_vector);

int user_num;
int movie_num;
map<int,double*> uvecs;
map<int,double*> mvecs;
vector<double> bias;
map<int,vector<int> >train;
vector<pair<int,int> >test;
map<int,map<int,int> >um_flag;
map<int,map<int,int> >test_map;
int train_num;
int test_num;

static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
static std::default_random_engine generator (seed);

bool mycompbytime(pair<int,long> a,pair<int,long> b) {
    return a.second<b.second?1:0;
}

void init() {
    FILE * ratings_file=fopen("data/ratings.dat","r");
    int user_id,movie_id;
    double temp;
    long tim;
    map<int,int> user_map;
    map<int,int> item_map;
    int new_uid=0;
    int new_iid=0;
    vector<vector<pair<int,long> > > ratings;
    int rating_num;
    while(fscanf(ratings_file,"%d",&user_id)!=EOF) {
        fscanf(ratings_file,"%d",&movie_id);
        fscanf(ratings_file,"%lf",&temp);
        fscanf(ratings_file,"%ld",&tim);
        if(user_map.find(user_id)!=user_map.end()) {
            user_id=user_map[user_id];
        }
        else {
            vector<pair<int,long> > tv;
            ratings.push_back(tv);
            user_map[user_id]=new_uid;
            user_id=new_uid;
            new_uid++;
        }
        if(item_map.find(movie_id)!=item_map.end()) {
            movie_id=item_map[movie_id];
        }
        else {
            item_map[movie_id]=new_iid;
            movie_id=new_iid;
            new_iid++;
        }
        ratings[user_id].push_back(make_pair(movie_id,tim));
        rating_num++;
    }
    fclose(ratings_file);
    user_num=new_uid;
    movie_num=new_iid;
    printf("users_num=%d\n",user_num);
    printf("movies_num=%d\n",movie_num);
    for(unsigned i=0; i<ratings.size(); i++) {
        sort(ratings[i].begin(),ratings[i].end(),mycompbytime);
    }

    for(unsigned i=0; i<ratings.size(); i++) {
	    int k_out=ratings[i].size()*split_into_test_ratio;
        for(int j=0; j<k_out; j++) {
            test.push_back(make_pair(i,ratings[i].back().first));
            ratings[i].pop_back();
        }
    }
    for(unsigned i=0; i<test.size(); i++) {
 		int test_uid=test[i].first;
 		int test_mid=test[i].second;
		if (test_map.count(test_uid)==0){
			test_map[test_uid]=map<int,int>();
		}
		test_map[test_uid][test_mid]=1;
	}
    for(int i=0; i<=new_uid; i++) {
        uvecs[i]=new double[dim];
        for(int j=0; j<dim; j++) {
            static std::normal_distribution<double> distu(0.0,1.0/dim);
            uvecs[i][j]=distu(generator);
        }
    }
    for(int i=0; i<=new_iid; i++) {
        mvecs[i]=new double[dim];
        for(int j=0; j<dim; j++) {
            static std::normal_distribution<double> distu(0.0,1.0/dim);
            mvecs[i][j]=distu(generator);
        }
    }
    for(unsigned i=0; i<ratings.size(); i++) {
        train[i]=vector<int>();
        um_flag[i]=map<int,int>();
        for(unsigned j=0; j<ratings[i].size(); j++) {
            train[i].push_back(ratings[i][j].first);
            um_flag[i][ratings[i][j].first]=1;
            train_num++;
        }
    }
    test_num=test.size();
    bias.resize(new_uid);
    printf("train_num: %d\n",train_num);
    printf("test_num=%d\n",test_num);
}

void update_one() {
    static std::uniform_int_distribution<int> distu(0,user_num-1);
    static std::uniform_int_distribution<int> disti(0,movie_num-1);
    static std::uniform_int_distribution<int> distj(0,movie_num-1);
    int u=distu(generator);
    int i=disti(generator)%train[u].size();
    i=train[u][i];
    int j=distj(generator);
    while(um_flag[u].count(j)!=0) {
        j=distj(generator);
    }
    double* uv=uvecs[u];
    double* miv=mvecs[i];
    double* mjv=mvecs[j];
    double *cuv=new double[dim](),*cmiv=new double[dim](),*cmjv=new double[dim]();
    for(int dimi=0;dimi<dim;dimi++){
        cuv[dimi]=uv[dimi];
        cmiv[dimi]=miv[dimi];
        cmjv[dimi]=mjv[dimi];
    }
    double xuij=bias[i]-bias[j]+vdot(uv,miv)-vdot(uv,mjv);
    double e_x=exp(xuij);
    double mult=1.0/(1.0+e_x);
    bias[i]+=learning_rate*(mult-lamda_bias*bias[i]);
    bias[j]+=learning_rate*(-mult-lamda_bias*bias[j]);
    for(int dimi=0; dimi<dim; dimi++) {
        uv[dimi]+=(learning_rate*(mult*(cmiv[dimi]-cmjv[dimi])-lamda_u*cuv[dimi]));
        miv[dimi]+=(learning_rate*(mult*cuv[dimi]-lamda_i*cmiv[dimi]));
        mjv[dimi]+=(learning_rate*(mult*(-cuv[dimi])-lamda_j*cmjv[dimi]));
    }
    delete[]cuv;delete[]cmiv;delete[]cmjv;
}
double update(){
    for(int iter=0; iter<train_num/10; iter++) {
        update_one();
    }
    return 0.0;
}

double auc() {
    static std::uniform_int_distribution<int> distu(0,user_num-1);
    static std::uniform_int_distribution<int> disti(0,movie_num-1);
    static std::uniform_int_distribution<int> distj(0,movie_num-1);
    double hit_count=0.0;
    int iternum=10000;
    int aucnum=0;
    for(int iter=0; iter<iternum; iter++) {
        int u=distu(generator);//test[randt].first;
        int i=disti(generator)%train[u].size();
        i=train[u][i];
        int j=distj(generator);
        while(um_flag[u].count(j)!=0) {
            j=distj(generator);
        }
        double*uv=uvecs[u];
        double doti=vdot(uv,mvecs[i]);
        double dotj=vdot(uv,mvecs[j]);
        if(doti>dotj)hit_count+=1;
        aucnum++;
    }
    return hit_count/aucnum;
}
bool mycompbyscore(pair<int,double>a,pair<int,double>b) {
    return a.second>b.second?1:0;
}
double hit(int uid,int mid) {
    double*uv=uvecs[uid];
    double*mv=mvecs[mid];
    double score=vdot(uv,mv);
    int largernum=0;
    vector<pair<int,double> > scores;
    for(int i=0; i<movie_num; i++) {
        double mscore=vdot(uv,mvecs[i]);
        scores.push_back(make_pair(i,mscore));
        if(mscore>score) {
            largernum++;
            if(largernum>topk)return 0.0;
        }
    }
    /*sort(scores.begin(),scores.end(),mycompbyscore);
    for(int i=0; i<topk; i++) {
        if(scores[i].first==mid)return 1.0;
    }
    return 0.0;*/
    return 1.0;
}
double hit_ratio() {
    static std::uniform_int_distribution<int> distt(0,test_num-1);
    double hit_count=0.0;
    int iternum=10000;
    int hit_iter=0;
    for(int iter=0; iter<iternum; iter++) {
        int randt=distt(generator);
        int u=test[randt].first;
        int i=test[randt].second;
        hit_count+=hit(u,i);
        hit_iter++;
    }
    //printf("hit_iter=%d hitcount=%8f ",hit_iter,hit_count);
    return hit_count/hit_iter;
}
double apk(int uid){	    
	if(test_map.count(uid)==0)return 0.0;
	double*uv=uvecs[uid];
	vector<pair<int,double> > scores;
    for(int i=0; i<movie_num; i++) {
        double*tmv=mvecs[i];
        double mscore=vdot(uv,tmv);
        scores.push_back(make_pair(i,mscore));
    }
    sort(scores.begin(),scores.end(),mycompbyscore);
    double score = 0.0;
    double num_hits = 0.0;
    for(int i=0;i<topk;i++){
        if (test_map[uid].count(scores[i].first)!=0){
            num_hits += 1.0;
            score += num_hits / (i+1.0);
        }
    }
    double k_temp=test_map[uid].size()>topk?topk:test_map[uid].size();
    return score / k_temp;
}
double mapk(){
    static std::uniform_int_distribution<int> disu(0,user_num-1);
    double mapk_score=0.0;
	for(int iter=0;iter<10000;iter++) {
        int randu=disu(generator);
        mapk_score+=apk(randu);
    }
    return mapk_score/10000;
}
double mapk_all(){
    double mapk_score=0.0;
	for(int iter=0;iter<user_num;iter++) {
        mapk_score+=apk(iter);
    }
    return mapk_score/user_num;
}
int main() {
    FILE* outputfile=fopen("output/out_put.txt","w");
    printf("learning_rate: %8f\n",learning_rate);
    printf("dim: %d\n",dim);
    printf("topk: %d\n",topk);
    printf("lamda_u: %8f\n",lamda_u);
    printf("lamda_i: %8f\n",lamda_i);
    printf("lamda_j: %8f\n",lamda_j);
    printf("lamda_bias: %8f\n",lamda_bias);
    //fprintf(outputfile,"learning_rate: %8f\n",learning_rate);
    init();
    char tmp[100];
    for(int iter=0; iter<maxiter; iter++) {
        cout<<iter<<"\t";
        //cout<<get_time(tmp)<<"\t";
        update();
        //cout<<get_time(tmp)<<"\t";
        //cout<<auc()<<"\t"<<hit_ratio()<<"\t"<<mapk()<<"\n";
        //cout<<0.0<<"\t"<<0.0<<"\t"<<mapk()<<"\n";
        cout<<0.0<<"\t"<<0.0<<"\t"<<mapk_all()<<"\n";
        //cout<<get_time(tmp)<<"\n";
    }
    fclose(outputfile);
    return 0;
}
double vdot(double* user_vector,double* movie_vector) {
    double s=0.0;
    for(int i=0; i<dim; i++) {
        s+=(user_vector[i]*movie_vector[i]);
    }
    return s;
}

char* get_time(char* tmp) {
    SYSTEMTIME sys;
    GetLocalTime( &sys );
    sprintf(tmp,"%02d:%02d:%02d:%03d",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds);
    return tmp;
}
