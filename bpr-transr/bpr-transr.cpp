/*
对于batch的理解是否有问题

要求：match中有的电影，ratings中一定有；ratings中有的电影，match中一定有。
*/
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

bool is_redirect_out=0;//是否重定向标准输出std到文件 

int dim=100;//15 100 10                        //向量维数 
double learning_rate=0.01;//0.03  0.03 0.01    //学习速率 
double lamda_bias=1;//0.1  0.1 0.1             //偏移项的正则化系数 
double lamda_u=0.01;//0.10  0.105 0.1075       //用户向量的正则化系数 
double lamda_i=0.01;//0.10  0.105 0.1075       //项目i特征向量的正则化系数 
double lamda_j=0.01;//0.10  0.105 0.1075       //项目j特征向量的正则化系数 
int topk=100;                                  //推荐列表长度 
int maxiter=1000;                              //最大迭代次数 
double split_into_test_ratio=0.3;              //输入数据中每个用户的评分数据中被拆分到test数据集的比例 

int rdim=200;//15 100 10                       //关系向量的维数 
double rlearning_rate=0.01;                    //transR方法的学习速率 
double lamda_e=0.1;                            //知识库实体的结构向量的正则化系数 
double lamda_r=0.1;                            //知识库关系向量的正则化系数 
double lamda_m=0.1;                            //TransR中投影矩阵的正则化系数 

double sigmoid(double x);                      //sigmoid函数 $\frac 1 {1+e^{-x}}$ 
char* get_time(char* tmp);                     //获取当前时间的字符串表示 

int user_num;                                  //用户数目 
int movie_num;                                 //电影数目 
int entity_num;                                //知识库实体的总数(包括电影实体) 
int relation_num;                              //关系数目 
map<int,double*> uvecs;                        //用户向量P_u的map 
map<int,double*> mvecs;                        //电影特征向量Q_i的map 
vector<double> bias;                           //偏移   共user_num个 
map<int,vector<int> >train;                    //训练数据  用户-电影 map 
vector<pair<int,int> >test;                    //测试数据  用户-电影 pair 
map<int,map<int,int> >um_flag;                 //有反馈时 用户-电影-1 ;无反馈时  用户-电影-0 
map<int,map<int,int> >test_map;                //有反馈时 用户-电影-1 ;无反馈时  用户-电影-0 
map<int,vector<pair<int,int> > >match;         //知识库数据  实体-vector<关系-实体>
map<int,map<int,map<int,int> > >match_map;     //知识库数据  存在三元组时  实体-关系-实体-1   否则0  
int train_num;                                 //训练数据的数目 
int test_num;                                  //测试数据的数目 
map<int,double*> evecs;                        //实体v_i的向量表示 包括电影实体 
map<int,double*> rvecs;                        //关系r的向量表示 
map<int,double**> m_matrices;                  //关系的投影矩阵M_r的map   关系-投影矩阵 

static unsigned seed =std::chrono::system_clock::now().time_since_epoch().count();  //随机数种子   C++11特性 
static std::default_random_engine generator (seed);                                 //随机数生成器   C++11特性 

// 通过时间给读取的评分数据排序时用到的comp函数    
bool mycompbytime(pair<int,long> a,pair<int,long> b) { 
    return a.second<b.second?1:0;
}
//将三元组放入match_map 
void put_in(int mid,int rid,int eid) {
    if(match_map.find(mid)!=match_map.end()) {
        if(match_map[mid].find(rid)==match_map[mid].end()) {
            match_map[mid][rid]=map<int,int>();
        }
    } else {
        match_map[mid]=map<int,map<int,int> >();
        match_map[mid][rid]=map<int,int>();
    }
    match_map[mid][rid][eid]=1;
    if(match.find(mid)==match.end()) {
        match[mid]=vector<pair<int,int> >();
    }
    match[mid].push_back(make_pair(rid,eid));
}
//计算向量内积 
double vdot(double* user_vector,double* movie_vector,int dim) {
    double s=0.0;
    for(int i=0; i<dim; i++) {
        s+=(user_vector[i]*movie_vector[i]);
    }
    return s;
}
//计算向量和 
void vadd(double* a,double* b,double* c,int dim) {
    for(int i=0; i<dim; i++) {
        a[i]=(b[i]+c[i]);
    }
}
//计算向量差 
void vsub(double* a,double* b,double* c,int dim) {
    for(int i=0; i<dim; i++) {
        a[i]=(b[i]-c[i]);
    }
}
//计算向量b和矩阵c的乘积，存入向量a   transR投影时用 
void vmul(double* a,double* b,double** c) {
    for(int i=0; i<rdim; i++) {
        a[i]=vdot(b,c[i],dim);
    }
}
//获取向量二范数的平方值 
double get_norm(double* x,int dim) {
    double s=0.0;
    for(int i=0; i<dim; i++) {
        s+=(x[i]*x[i]);
    }
    return s;
}
//初始化数据 
void init() {
    FILE * ratings_file=fopen("data/ratings.dat","r");
    int user_id,movie_id;
    double temp;
    long tim;
    map<int,int> user_map;       // 给读取的用户id重新赋值新的连续id 
    map<int,int> item_map;       // 给读取的电影项目id重新赋值新的连续的项目id 
    int new_uid=0;
    int new_iid=0;
    vector<vector<pair<int,long> > > ratings;//vector< vector< pair<电影-评分时间> > >  下标为user_id 
    int rating_num;
    while(fscanf(ratings_file,"%d",&user_id)!=EOF) {
        fscanf(ratings_file,"%d",&movie_id);
        fscanf(ratings_file,"%lf",&temp);
        fscanf(ratings_file,"%ld",&tim);
        if(user_map.find(user_id)!=user_map.end()) {
            user_id=user_map[user_id];
        } else {
            vector<pair<int,long> > tv;
            ratings.push_back(tv);
            user_map[user_id]=new_uid;
            user_id=new_uid;
            new_uid++;
        }
        if(item_map.find(movie_id)!=item_map.end()) {
            movie_id=item_map[movie_id];
        } else {
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
        sort(ratings[i].begin(),ratings[i].end(),mycompbytime);  // 对每个用户的所有评分数据按时间排序 
    }
    //把每个用户的评分数据中的split_into_test_ratio划为test数据 
    for(unsigned i=0; i<ratings.size(); i++) {
	    int k_out=ratings[i].size()*split_into_test_ratio;
        for(int j=0; j<k_out; j++) {
            test.push_back(make_pair(i,ratings[i].back().first));
            ratings[i].pop_back();
        }
    }
    //构造test_map 
    for(unsigned i=0; i<test.size(); i++) {
 		int test_uid=test[i].first;
 		int test_mid=test[i].second;
		if (test_map.count(test_uid)==0){
			test_map[test_uid]=map<int,int>();
		}
		test_map[test_uid][test_mid]=1;
	}
	//初始化用户向量P_u
    for(int i=0; i<=new_uid; i++) {
        uvecs[i]=new double[dim];
        for(int j=0; j<dim; j++) {
            static std::normal_distribution<double> distu(0.0,1.0/dim);
            uvecs[i][j]=distu(generator);
        }
    }
    //初始化电影向量Q_i 
    for(int i=0; i<=new_iid; i++) {
        mvecs[i]=new double[dim];
        for(int j=0; j<dim; j++) {
            static std::normal_distribution<double> distu(0.0,1.0/dim);
            mvecs[i][j]=distu(generator);
        }
    }
    //构造train和um_flag 
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
    //load match
    char buf[1024];
    string item_name,fact_id,e1_name,r_name,e2_name;
    FILE * match_file=fopen("data/match_results.txt","r");
    map<string,int> r_map;//给每个关系赋值新的连续id 
    map<string,int> e_map;//给每个实体赋值新的连续id   若是电影 则使用之前赋值后的电影id   电影id占据前movie_num个实体id 
    int r_id,e_id,new_rid=0;
    int match_num=0;
    while(fscanf(match_file,"%d",&movie_id)!=EOF) {
        fscanf(match_file,"%s",buf);
        item_name=buf;
        fscanf(match_file,"%s",buf);
        fact_id=buf;
        fscanf(match_file,"%s",buf);
        e1_name=buf;
        fscanf(match_file,"%s",buf);
        r_name=buf;
        fscanf(match_file,"%s",buf);
        e2_name=buf;
        transform(item_name.begin(), item_name.end(), item_name.begin(), towupper);//将字符串转换为大写 
        transform(e1_name.begin(), e1_name.end(), e1_name.begin(), towupper);
        transform(e2_name.begin(), e2_name.end(), e2_name.begin(), towupper);
        movie_id=item_map[movie_id];
        if(r_map.find(r_name)!=r_map.end()) {
            r_id=r_map[r_name];
        } else {
            r_map[r_name]=new_rid;
            r_id=new_rid;
            new_rid++;
        }
        if(item_name==e1_name) {//match 数据中的三元组可能是 电影-r实体 ，也可能是 实体-r-电影 
            if(e_map.find(e2_name)!=e_map.end()) {
                e_id=e_map[e2_name];
            } else {
                e_map[e2_name]=new_iid;
                e_id=new_iid;
                new_iid++;
            }
        } else { //if(item_name==e2_name){
            if(e_map.find(e1_name)!=e_map.end()) {
                e_id=e_map[e1_name];
            } else {
                e_map[e1_name]=new_iid;
                e_id=new_iid;
                new_iid++;
            }
        }
        put_in(movie_id,r_id,e_id);
        match_num++;
    }
    //初始化电影实体向量v_i 
    for(int mi=0; mi<movie_num; mi++) {
        double * td=new double[dim];
        for(int j=0; j<dim; j++) {
            static std::normal_distribution<double> diste(0.0,1.0/dim);
            td[j]=diste(generator);
        }
        evecs[mi]=td;
    }
    //初始化非电影的实体的实体向量v 
    for(map<string,int>::iterator it=e_map.begin(); it!=e_map.end(); it++) {
        double * td=new double[dim];
        for(int j=0; j<dim; j++) {
            static std::normal_distribution<double> diste(0.0,1.0/dim);
            td[j]=diste(generator);
        }
        evecs[(*it).second]=td;
    }
    //初始化关系向量和关系投影矩阵 
    for(map<string,int>::iterator it=r_map.begin(); it!=r_map.end(); it++) {
        double * td=new double[rdim];
        for(int j=0; j<rdim; j++) {
            static std::normal_distribution<double> distrd(0.0,1.0/rdim);
            td[j]=distrd(generator);
        }
        rvecs[it->second]=td;
        double ** tm=new double*[rdim];
        for(int in=0; in<rdim; in++) {
            tm[in]=new double[dim]();
            for(int im=0; im<dim; im++) {
                if(in==im)tm[in][im]=1.0;
                else tm[in][im]=0.0;
            }
        }
        m_matrices[it->second]=tm;
    }
    entity_num=new_iid;
    relation_num=new_rid;
    cout<<"match_num="<<match_num<<endl;
    cout<<"e_num="<<entity_num<<endl;
    cout<<"r_num="<<relation_num<<endl;
}
//求导的中间结果 
double get_ei(double* eri,double** m,int f) {
    double x=0.0;
    for(int i=0; i<rdim; i++) {
        x+=((-2)*eri[i]*m[i][f]);
    }
    return x;
}
//求导的中间结果 
double get_ej(double* erj,double** m,int f) {
    double x=0.0;
    for(int i=0; i<rdim; i++) {
        x+=(2*erj[i]*m[i][f]);
    }
    return x;
}
//求导的中间结果 
double get_ee(double* eri,double* erj,double** m,int f) {
    double x=0.0;
    for(int i=0; i<rdim; i++) {
        x+=(2*(eri[i]-erj[i])*m[i][f]);
    }
    return x;
}
//迭代更新一个 
int update_one() {
	//随机获取(u,i,j)三元组  u和i有反馈  u和j没有反馈 
    static std::uniform_int_distribution<int> distu(0,user_num-1);
    static std::uniform_int_distribution<int> disti(0,movie_num-1);
    static std::uniform_int_distribution<int> distj(0,movie_num-1);
    static std::uniform_int_distribution<int> distuv(0,entity_num-1);
    int u=distu(generator);
    int i=disti(generator)%train[u].size();
    i=train[u][i];
    int j=distj(generator);
    int try_num=0;
    while(um_flag[u].count(j)!=0&&try_num<10) {
        j=distj(generator);
        try_num++;
    }
    if(try_num>=10)return 0;
    int rid,eid;
    //构造四元组(e,r,i,j) 
    for(try_num=0; try_num<10; try_num++) {
        int rand_match_u=distuv(generator)%match[i].size();
        rid=match[i][rand_match_u].first;
        eid=match[i][rand_match_u].second;
        if(match_map[j].count(rid)==0)continue;
        if(match_map[j][rid].count(eid)!=0)continue;
        break;
    } 
    if(try_num>=10)return 0;
    double* uv=uvecs[u];
    double* miv=mvecs[i];
    double* mjv=mvecs[j];
    double* eiv=evecs[i];
    double* ejv=evecs[j];
    double* eev=evecs[eid];
    double* rv=rvecs[rid];
    double** m_matrix=m_matrices[rid];
    //构造副本  以不同步更新 
    double cuv[dim]= {0.0},cmiv[dim]= {0.0},cmjv[dim]= {0.0},ceiv[dim]= {0.0},cejv[dim]= {0.0},ceev[dim]= {0.0},crv[rdim]= {0.0};
    double** cm_matrix=new double*[rdim];
    for(int dimi=0; dimi<rdim; dimi++) {
        cm_matrix[dimi]=new double[dim]();
    }
    for(int dimi=0; dimi<dim; dimi++) {
        cuv[dimi]=uv[dimi];
        cmiv[dimi]=miv[dimi];
        cmjv[dimi]=mjv[dimi];
        ceiv[dimi]=eiv[dimi];
        cejv[dimi]=ejv[dimi];
        ceev[dimi]=eev[dimi];
    }
    for(int dimi=0; dimi<rdim; dimi++) {
        crv[dimi]=rv[dimi];
        for(int dimj=0; dimj<dim; dimj++) {
            cm_matrix[dimi][dimj]=m_matrix[dimi][dimj];
        }
    }
    //e_i=Q_i+v_i 
    double ei[dim]= {0.0},ej[dim]= {0.0};
    vadd(ei,cmiv,ceiv,dim);
    vadd(ej,cmjv,cejv,dim);
    double xuij=bias[i]-bias[j]+vdot(cuv,ei,dim)-vdot(cuv,ej,dim);
    double e_x=exp(xuij);
    double mult=1.0/(1.0+e_x);
    bias[i]+=learning_rate*(mult-lamda_bias*bias[i]);
    bias[j]+=learning_rate*(-mult-lamda_bias*bias[j]);
    double em[rdim]= {0.0},im[rdim]= {0.0},jm[rdim]= {0.0},emaddr[rdim]= {0.0},eri[rdim]= {0.0},erj[rdim]= {0.0};
    vmul(em,ceev,cm_matrix);
    vmul(im,ceiv,cm_matrix);
    vmul(jm,cejv,cm_matrix);
    vadd(emaddr,em,crv,rdim);
    vsub(eri,emaddr,im,rdim);
    vsub(erj,emaddr,jm,rdim);
    double fsubf=get_norm(eri,rdim)-get_norm(erj,rdim);
    double e_f=exp(fsubf);
    double multf=1.0/(1.0+e_f);
	//更新 
    for(int dimi=0; dimi<dim; dimi++) {
        uv[dimi]+=(learning_rate*(mult*(ei[dimi]-ej[dimi])-lamda_u*cuv[dimi]));
        miv[dimi]+=(learning_rate*(mult*cuv[dimi]-lamda_i*cmiv[dimi]));
        mjv[dimi]+=(learning_rate*(mult*(-cuv[dimi])-lamda_j*cmjv[dimi]));
        eiv[dimi]+=(rlearning_rate*(mult*cuv[dimi]+multf*get_ei(eri,cm_matrix,dimi)-lamda_e*ceiv[dimi]));
        ejv[dimi]+=(rlearning_rate*(mult*(-cuv[dimi])+multf*get_ej(erj,cm_matrix,dimi)-lamda_e*cejv[dimi]));
        eev[dimi]+=(rlearning_rate*(multf*get_ee(eri,erj,cm_matrix,dimi)-lamda_e*ceev[dimi]));
    }
    for(int dimi=0; dimi<rdim; dimi++) {
        rv[dimi]+=(rlearning_rate*(multf*2*(eri[dimi]-erj[dimi])-lamda_r*crv[dimi]));
    }
    for(int dimi=0; dimi<rdim; dimi++) {
        for(int dimj=0; dimj<dim; dimj++) {
            m_matrix[dimi][dimj]+=(rlearning_rate*(multf*2*(eri[dimi]*(ceev[dimj]-ceiv[dimj])-erj[dimi]*(ceev[dimj]-cejv[dimj]))-lamda_m*cm_matrix[dimi][dimj]));
        }
    }
    for(int dimi=0; dimi<rdim; dimi++) {
        delete [] cm_matrix[dimi];
    }
    delete [] cm_matrix;
    return 1;
}
//一次大的迭代更新 
double update() {
    int iter=0;
    while(iter<train_num/10) {
        iter+=update_one();
        //cout<<iter<<endl;
    }
    return 0.0;
}
//计算AUC指标 
double auc() {
    static std::uniform_int_distribution<int> distu(0,user_num-1);
    static std::uniform_int_distribution<int> disti(0,movie_num-1);
    static std::uniform_int_distribution<int> distj(0,movie_num-1);
    double hit_count=0.0;
    int iter=0,iternum=10000;
    while(iter<iternum) {
        int u=distu(generator);//test[randt].first;
        int i=disti(generator)%train[u].size();
        i=train[u][i];
        int j=distj(generator);
        int try_num=0;
        while(um_flag[u].count(j)!=0&&try_num<10) {
            j=distj(generator);
            try_num++;
        }
        if(try_num>=10)continue;
        double*uv=uvecs[u];
        double*miv=mvecs[i];
        double*mjv=mvecs[j];
        double*eiv=evecs[i];
        double*ejv=evecs[j];
        double ei[dim]= {0.0},ej[dim]= {0.0};
        vadd(ei,miv,eiv,dim);
        vadd(ej,mjv,ejv,dim);
        double doti=vdot(uv,ei,dim);
        double dotj=vdot(uv,ej,dim);
        if(doti>dotj)hit_count+=1;
        iter++;
    }
    return hit_count/iternum;
}
//通过分数排序的comp函数 
bool mycompbyscore(pair<int,double>a,pair<int,double>b) {
    return a.second>b.second?1:0;
}
//对于uid,mid，判断是否hit ，hit返回1，否则返回0 
double hit(int uid,int mid) {
    double*uv=uvecs[uid];
    double*mv=mvecs[mid];
    double*ev=evecs[mid];
    double e[dim]= {0.0};
    vadd(e,mv,ev,dim);
    double score=vdot(uv,e,dim);
    int largernum=0;
    vector<pair<int,double> > scores;
    for(int i=0; i<movie_num; i++) {
        double*tev=evecs[i];
        double*tmv=mvecs[i];
        double te[dim]= {0.0};
        vadd(te,tmv,tev,dim);
        double mscore=vdot(uv,te,dim);
        scores.push_back(make_pair(i,mscore));
        if(mscore>score) {
            largernum++;
            if(largernum>topk)return 0.0;
        }
    }
    sort(scores.begin(),scores.end(),mycompbyscore);
    /*for(int i=0; i<topk; i++) {
        if(scores[i].first==mid)return 1.0;
    }
    return 0.0;*/
    return 1.0;
}
//计算hit_ratio，即推荐命中准确率 
double hit_ratio() {
    static std::uniform_int_distribution<int> distt(0,test_num-1);
    double hit_count=0.0;
    int iternum=10000;
    int iter=0;
    while(iter<iternum) {
        int randt=distt(generator);
        int u=test[randt].first;
        int i=test[randt].second;
        hit_count+=hit(u,i);
        iter++;
    }
    //printf("hit_iter=%d hitcount=%8f ",hit_iter,hit_count);
    return hit_count/iternum;
}
//计算一个用户的apk指标值 
double apk(int uid){	    
	if(test_map.count(uid)==0)return 0.0;
	double*uv=uvecs[uid];
	vector<pair<int,double> > scores;
    for(int i=0; i<movie_num; i++) {
        double*tev=evecs[i];
        double*tmv=mvecs[i];
        double te[dim]= {0.0};
        vadd(te,tmv,tev,dim);
        double mscore=vdot(uv,te,dim);
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
//计算中的map@K指标值   随机选取10000个用户 
double mapk(){
    static std::uniform_int_distribution<int> disu(0,user_num-1);
    double mapk_score=0.0;
	for(int iter=0;iter<10000;iter++) {
        int randu=disu(generator);
        mapk_score+=apk(randu);
    }
    return mapk_score/10000;
}
//计算map@K指标值，对所有用户
double mapk_all(){
    double mapk_score=0.0;
	for(int iter=0;iter<user_num;iter++) {
        mapk_score+=apk(iter);
    }
    return mapk_score/user_num;
}
int main() {
    if(is_redirect_out) {
        freopen("C:/Users/xuewei/Desktop/out.txt","w",stdout);
    }
    printf("learning_rate: %8f\n",learning_rate);
    printf("dim: %d\n",dim);
    printf("topk: %d\n",topk);
    printf("lamda_u: %8f\n",lamda_u);
    printf("lamda_i: %8f\n",lamda_i);
    printf("lamda_j: %8f\n",lamda_j);
    printf("lamda_bias: %8f\n",lamda_bias);
    printf("\nrlearning_rate: %8f\n",rlearning_rate);
    printf("rdim: %d\n",rdim);
    printf("lamda_e: %8f\n",lamda_e);
    printf("lamda_r: %8f\n",lamda_r);
	printf("lamda_m: %8f\n",lamda_m);
    init();
    char tmp[100];
    maxiter=501;
    double xxxxx=0.0;
    for(int iter=0; iter<maxiter; iter++) {
        cout<<iter<<"\t";
//        cout<<get_time(tmp)<<"\t";
        update();
        //cout<<get_time(tmp)<<"\t";
//        cout<<auc()<<"\t"<<hit_ratio()<<"\t"<<mapk()<<"\n";
	//	if(iter>40||iter%10==0)
		if(iter%10==0||xxxxx>0.046){
			xxxxx=mapk_all();
  			cout<<0.0<<"\t"<<0.0<<"\t"<<xxxxx<<"\n";
  		}
		else
	        cout<<0.0<<"\t"<<0.0<<"\t"<<0.0<<"\n";

        //cout<<get_time(tmp)<<"\n";
        fflush(stdout);
    }

    if(is_redirect_out) {
        fclose(stdout);
    }
    return 0;
}

char* get_time(char* tmp) {
    SYSTEMTIME sys;
    GetLocalTime( &sys );
    sprintf(tmp,"%02d:%02d:%02d:%03d",sys.wHour,sys.wMinute,sys.wSecond,sys.wMilliseconds);
    return tmp;
}

double sigmoid(double x) {
    return 1.0/(1+exp(-x));
}
