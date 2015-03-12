//O(nk^2mlogm+nk^2)
//nΪ�켣����
//mΪ·����
//kΪĳ���켣��ĺ�ѡ����

#include "stdafx.h"
using namespace std;

struct Date{
	int year,month,day;
	int hour,minute,second;
	static const int daySec = 3600*24;
	explicit Date(int y,int mon,int d,int h,int min,int sec):year(y),month(mon),day(d),hour(h),minute(min),second(sec){}
	//�����켣���¼ʱ��Ĳ�ֵ�����Խһ��

	int operator - (const Date& rhs) const{
		int s1 = hour*3600+minute*60+second;
		int s2 = rhs.hour*3600+rhs.minute*60+rhs.second;
		if(year == rhs.year && month == rhs.month && day == rhs.day)
			return abs(s1-s2);
		else
			return abs(daySec - abs(s1-s2));
	}
	bool operator == (const Date& rhs) const{
		return year == rhs.year && month == rhs.month && day == rhs.day && hour == rhs.hour && minute == rhs.minute && second == rhs.second;
	}
};

struct GeoPoint{
	double latitude,longitude;//γ�ȣ�����
	Date date;//���ں�ʱ��
	GeoPoint(double lat,double lon,Date dat):latitude(lat),longitude(lon),date(dat){}
};

string dbname;//���ݿ�����
string dbport;//���ݿ�˿ں�
string dbaddr;//���ݿ��ַ
string roadTN;//��·������
int threadnum;//���ڼ�����߳�����
int top;//������·��ʱѡȡ��ǰֵ����TOP����������һ����ļ���
double R = 0.1;//ѡȡĳ�켣��ĺ�ѡ��ķ�Χ����λΪKm
double Sigma = 0.02;//��̬�ֲ�����λΪKm
int K = 5;//��ѡ����������

//////////////////��������/////////////////////////
//variable
time_t tm;//��ʱ����
double Coef;//norm distribution coef
vector <GeoPoint> P;//�켣��
vector < vector <Point> > candiPoint;//ÿ���켣��ĺ�ѡ�㼯��
//Fs�Ƕ�̬�滮�к�ѡ��ת��ʱ�ռ���ۣ�Ft��ʱ����ۣ�F���ܴ��ۡ�F = Fs*Ft
map < pair<int,int> , double > F;

Database *DB;//���ݿ�����ʵ��
Graph *network;

//////////////////End/////////////////////////

//�ѹ켣��������ݿ�
void loadInitPoint(){
	string SQL = "select * from pg_class where relname = 'init_point'";
	PGresult* res = DB->execQuery(SQL);
	int num = PQntuples(res);
	PQclear(res);

	if(num != 0){
		SQL = "Delete from init_point";
		DB->execUpdate(SQL);
	}
	else{
		SQL = "create table init_point (id integer primary key,year integer,month integer,day integer,hour integer,minute integer,second integer,way geometry(Point,4326))";
		DB->execUpdate(SQL);
	}
	size_t sz = P.size();
	char buffer[500];

	for(int i=0;i<sz;++i){
		sprintf_s(buffer,"insert into init_point values(%d,%d,%d,%d,%d,%d,%d,ST_GeomFromText('Point(%lf %lf)',4326))",i+1,P[i].date.year,P[i].date.month,P[i].date.day,P[i].date.hour,P[i].date.minute,P[i].date.second,P[i].longitude,P[i].latitude);
		SQL = buffer;
		DB->execUpdate(SQL);
	}
}

//�Ѻ�ѡ��������ݿ�
void loadCandiPoint(){
	string SQL = "select * from pg_class where relname = 'candi_point'";
	PGresult* res = DB->execQuery(SQL);
	int num = PQntuples(res);
	PQclear(res);

	if(num != 0){
		SQL = "Delete from candi_point";
		DB->execUpdate(SQL);
	}
	else{
		SQL = "create table candi_point (id integer primary key,belong integer,way geometry(Point,4326))";
		DB->execUpdate(SQL);
	}
	size_t sz = candiPoint.size();
	char buffer[500];

	for(int i=0;i<sz;i++){
		size_t candisz = candiPoint[i].size();
		for(int j=0;j<candisz;++j){
			sprintf_s(buffer,"insert into candi_point values(%d,%d,ST_GeomFromText('Point(%lf %lf)',4326))",candiPoint[i][j].id,i+1,candiPoint[i][j].x,candiPoint[i][j].y);
			SQL = buffer;
			DB->execUpdate(SQL);
		}
	}
}

//�����ݿ��ж�����LINESTRING�ַ���ת��Ϊ������
//���ص��vector
vector < pair<double,double> > parseString(string str){
	vector < pair<double,double> > res;
	size_t len = str.size();
	int pre = 11;
	double x,y;
	for(int i=0;i<len;++i){
		if(str[i] == ',' || str[i] == ')'){
			string tmp = str.substr(pre,i-pre);
			//cerr<<tmp<<endl;
			sscanf_s(tmp.c_str(),"%lf %lf",&x,&y);
			res.push_back(make_pair(x,y));
			pre = i+1;
		}
	}
	return res;
}

bool getTaxiTrajectory(string filePath){
	ifstream fin;
	fin.open(filePath);
	if(!fin) {
		cerr<<"File open error!"<<endl;
		fin.close();
		return false;
	}
	else{
		double lat,lon;
		long long carID;
		int y,mon,d,h,min,s;
		char buff[100];
		while(fin.getline(buff,100)){
			sscanf_s(buff,"%lld,%d-%d-%d %d:%d:%d,%lf,%lf",&carID,&y,&mon,&d,&h,&min,&s,&lon,&lat);
			P.push_back(GeoPoint(lat,lon,Date(y,mon,d,h,min,s)));
		}
	}
	fin.close();
	candiPoint.resize(P.size());
	return true;
}

//��ʼ��
//����켣���������ӳ��
bool init(string basePath){
	//network.reset();
	Coef = 1/(sqrt(2*PI)*Sigma);
	P.clear();
	candiPoint.clear();
	F.clear();
	network->reset();
	//splitTrajectory(basePath);
	return getTaxiTrajectory(basePath);
}

//��̬�ֲ�
double N(GeoPoint p,Point candiPoint){
	double x = getGeoDis(Point(p.longitude,p.latitude),candiPoint);
	return Coef*exp(-SQR(x)/(2*SQR(Sigma)));
}

//Transmission Probability
//if t == s then return 1; because t must transmit s
double V(double d,Point t,Point s){
	if(t == s) return 1;
	return d/network->getCandiShortest(t,s);
}

//����켣��ÿ����ĺ�ѡ�㼯��
//����F,Fs,Ft
//#define FT
vector <Point> FindMatchedSequence(){
	tm = clock();
	cerr<<"start FindMatchedSequence"<<endl;
	
	double Fs,Ft;

	int totCandiPoint = network->totCandiPoint;

	double *f = new double[totCandiPoint+1];
	int *preVertex = new int[totCandiPoint+1];

	for(int i=0;i<=totCandiPoint;++i)
		f[i] = 0,preVertex[i] = 0;

	struct QStore{
		double f;
		int idx;
		QStore(double _f,int _idx):f(_f),idx(_idx){}
		bool operator < (const QStore& x)const{
			return f > x.f;
		}
	};
	vector < QStore > Q;//fֵ��candiPoint[i-1]���±�
	size_t sz = candiPoint[0].size();
	for(int i=0;i<sz;++i){
		f[candiPoint[0][i].id] = N(P[0],candiPoint[0][i]);
		Q.push_back(QStore(f[candiPoint[0][i].id],i));
	}

	size_t Size = candiPoint.size();
	for(int i=1;i<Size;++i){
		size_t curSize = candiPoint[i].size();
		size_t preSize = Q.size();
		double d = getGeoDis(Point(P[i].longitude,P[i].latitude),Point(P[i-1].longitude,P[i-1].latitude));
		
		for(int k=0;k<curSize;++k){
			double Max = -DBL_MAX_10_EXP;
			Point cur(candiPoint[i][k]);
			for(int j=0;j<preSize;++j){
				Point pre(candiPoint[i-1][Q[j].idx]);
				
				//��V�Ĺ����л���w��ͬʱ�õ�(t->s)������·�μ�
				network->tTosMin = DBL_MAX;
				network->tTosSeg = 0;
				//ͨ����ѡ��id����ʾһ����
				pair <int,int> x = make_pair(pre.id,cur.id);
				Fs = N(P[i],cur)*V(d,pre,cur);
				//Ft is cosine distance
				//if t and s is same Point,then Ft must be 1
				//if t can reach s,then Ft must be 0
#ifdef FT
				if(pre == cur){
					Ft = 1;
				}
				else if(network.tTosMin == DBL_MAX){
					Ft = 0;
				}
				else{
					vector <double> speed = network.getSpeed();
					double vMean = 0;//ƽ���ٶ�
					double numerator = 0;
					double vv1 = 0 , vv2 = 0;

					vMean = network.tTosMin*1000/(P[i].date - P[i-1].date);//m/s
					int spSize = (int)speed.size();//return m/s
					for(int it=0;it<spSize;++it){
						numerator += vMean*speed[it];
						vv1 += SQR(speed[it]);
						vv2 += SQR(vMean);
					}
					double vv = sqrt(vv1)*sqrt(vv2);
					Ft = numerator / vv;
				}
				F[x] = Fs*Ft;
#else
				F[x] = Fs;
#endif
				double alt = f[pre.id]+F[x];
				if(alt > Max){
					Max = alt;
					preVertex[cur.id] = pre.id;
				}
			}
			f[cur.id] = Max;
		}
		Q.clear();
		for(int k=0;k<curSize;++k) 
			Q.push_back(QStore(f[candiPoint[i][k].id],k));
		sort(Q.begin(),Q.end());
		while(Q.size() > top) Q.pop_back();
	}
	vector <Point> res;
	double Max = -DBL_MAX_10_EXP;
	int c = 0;
	sz = candiPoint[Size-1].size();
	for(int i=0;i<sz;++i){
		if(Max < f[candiPoint[Size-1][i].id]){
			Max = f[candiPoint[Size-1][i].id];
			c = candiPoint[Size-1][i].id;
		}
	}
	for(int i=(int)Size-1;i>0;--i){
		res.push_back(network->getCandiPointById(c));
		c = preVertex[c];
	}
	res.push_back(network->getCandiPointById(c));

	delete []f;
	delete []preVertex;
	reverse(res.begin(),res.end());
	cerr<<"FindMatchedSequence cost = "<<clock()-tm<<"ms"<<endl;
	return res;
}

//ST��Matching�㷨

mutex lock_it;
int it;
DWORD WINAPI calc_candiPoint(LPVOID ptr){
	int upd = (int)P.size();
	int cur;
	while(true){
		lock_it.lock();
		if(it >= upd) {
			lock_it.unlock();
			return 0;
		}
		cur = it;
		++it;
		lock_it.unlock();

		vector <Point> tmp = network->getCandidate(Point(P[cur].longitude,P[cur].latitude),R,K);
		
		candiPoint[cur] = tmp;
	}
}

vector <Point> ST_Matching_Algorithm(){
	
	//O(nm) for every point in Trajectory find the candipoint set
	cerr<<"start getCandiPoint"<<endl;
	tm = clock();
	it= 0;
	HANDLE handle[threadNum];
	for(int i=0;i<threadNum;++i){
		handle[i] = CreateThread(NULL,0,calc_candiPoint,NULL,0,NULL);
	}
	for(int i=0;i<threadNum;++i){
		WaitForSingleObject(handle[i],INFINITE);
	}

	cerr<<"getCandidate cost = "<<clock()-tm<<"ms"<<endl;
	loadCandiPoint();

	return FindMatchedSequence();
}

//��ƥ��ѡ�еĵ㣬·��д�����ݿ�
void writeToDB(vector <Point> Traj){
	//д��ѡ�еĺ�ѡ��
	string SQL = "select * from pg_class where relname = 'trajectory_point'";
	PGresult* res = DB->execQuery(SQL);
	int num = PQntuples(res);
	PQclear(res);

	if(num != 0){
		SQL = "Delete from trajectory_point";
		DB->execUpdate(SQL);
	}
	else{
		SQL = "create table trajectory_point (id integer primary key,way geometry(Point,4326))";
		DB->execUpdate(SQL);
	}
	size_t sz = Traj.size();
	char buffer[500];

	for(int i=0;i<sz;++i){
		sprintf_s(buffer,"insert into trajectory_point values(%d,ST_GeomFromText('Point(%lf %lf)',4326))",i+1,Traj[i].x,Traj[i].y);
		SQL = buffer;
		DB->execUpdate(SQL);
	}

	//д��ƥ��Ĺ켣
	SQL = "select * from pg_class where relname = 'trajectory_line'";
	res = DB->execQuery(SQL);
	num = PQntuples(res);
	PQclear(res);

	if(num != 0){
		SQL = "Delete from trajectory_line";
		DB->execUpdate(SQL);
	}
	else{
		SQL = "create table trajectory_line (id integer primary key,src integer,des integer,way geometry(LineString,4326))";
		DB->execUpdate(SQL);
	}
	sz = Traj.size();
	buffer[500];

	int ID = 0;
	for(int i=1;i<sz;++i){
		if(network->isInSameSeg(Traj[i-1].id,Traj[i].id)){
			//in same seg
			sprintf_s(buffer,"insert into trajectory_line values(%d,%d,%d,ST_GeomFromText('LineString(%lf %lf,%lf %lf)',4326))",ID++,Traj[i-1].id,Traj[i].id,Traj[i-1].x,Traj[i-1].y,Traj[i].x,Traj[i].y);
			SQL = buffer;
			DB->execUpdate(SQL);
		}
		else{
			vector <int> path = network->getPath(Traj[i-1],Traj[i]);
			path.pop_back();//path�а���S,T������λ��һͷһβ
			if(path.empty() || path.size() < 2) {
				sprintf_s(buffer,"insert into trajectory_line values(%d,%d,%d,ST_GeomFromText('LineString(%lf %lf,%lf %lf)',4326))",ID++,Traj[i-1].id,Traj[i].id,Traj[i-1].x,Traj[i-1].y,Traj[i].x,Traj[i].y);
				SQL = buffer;
				DB->execUpdate(SQL);
			}
			else{
				Point cur(network->getPointById(path[1]));
				sprintf_s(buffer,"insert into trajectory_line values(%d,%d,%d,ST_GeomFromText('LineString(%lf %lf,%lf %lf)',4326))",ID++,Traj[i-1].id,Traj[i].id,cur.x,cur.y,Traj[i].x,Traj[i].y);
				SQL = buffer;
				DB->execUpdate(SQL);
				size_t psz = path.size();
				for(int j=2;j<psz;++j){
					cur = network->getPointById(path[j]);
					Point pre(network->getPointById(path[j-1]));
					sprintf_s(buffer,"insert into trajectory_line values(%d,%d,%d,ST_GeomFromText('LineString(%lf %lf,%lf %lf)',4326))",ID++,Traj[i-1].id,Traj[i].id,cur.x,cur.y,pre.x,pre.y);
					SQL = buffer;
					DB->execUpdate(SQL);
				}
				cur = network->getPointById(path[psz-1]);
				sprintf_s(buffer,"insert into trajectory_line values(%d,%d,%d,ST_GeomFromText('LineString(%lf %lf,%lf %lf)',4326))",ID++,Traj[i-1].id,Traj[i].id,Traj[i-1].x,Traj[i-1].y,cur.x,cur.y);
				SQL = buffer;
				DB->execUpdate(SQL);
			}

		}
	}
}

//����ԭʼ���ݣ���LineString�к��ж���������Ĳ��ֲ�ֺ�������ݿ�
//ֻ��Ҫ����һ��

int Insert(string SQL,int id){
	PGresult* res = DB->execQuery(SQL);

	int tupleNum = PQntuples(res);
	int fieldNum = PQnfields(res);

	for(int i=0;i<tupleNum;i++){
		string content = PQgetvalue(res,i,fieldNum-1);
		vector < pair<double,double> > pts = parseString(content);
		size_t sz = pts.size();
		for(int j=1;j<sz;j++){

			SQL = "insert into network values(";
			SQL += std::to_string(id++)+",0,0,";//id,source,target
			SQL += PQgetvalue(res,i,0);//osm_id
			SQL += ",";
			for(int k=1;k<fieldNum;k++){

				char * ss = PQgetvalue(res,i,k);
				if(strlen(ss) == 0) 
					SQL+="NULL";
				else{
					char * name = PQfname(res,k);
					if(strcmp(name,"name") == 0 || strcmp(name,"tags") == 0){
						SQL += "$$";
						SQL += ss;
						SQL += "$$";
					}
					else if(strcmp(name,"way") == 0){
						SQL += "ST_geomFromText('LINESTRING(";
						SQL += std::to_string(pts[j-1].first);
						SQL += " ";
						SQL += std::to_string(pts[j-1].second);
						SQL += ",";
						SQL += std::to_string(pts[j].first);
						SQL += " ";
						SQL += std::to_string(pts[j].second);
						SQL += ")',900913)";
					}
					else{
						SQL += "'";
						SQL += ss;
						SQL += "'";
					}
				}
				if(k+1 == fieldNum) break;
				SQL += ",";
			}
			SQL += ")";
			//cerr<<SQL<<endl;
			DB->execUpdate(SQL);
		}
	}

	PQclear(res);
	return id;
}

void preProcData(){

	string SQL = "select * from pg_class where relname = 'network'";
	PGresult* res = DB->execQuery(SQL);
	int num = PQntuples(res);
	PQclear(res);

	if(num != 0){
		SQL = "Delete from network";
		DB->execUpdate(SQL);
	}
	else{
		SQL = "";
		ifstream fin;
		fin.open("createNetwork.txt");
		string tmp;
		while(fin>>tmp) SQL += tmp;
		fin.close();
		DB->execUpdate(SQL);
	}

	string Field = "";
	SQL = "select * from allroads";
	res = DB->execQuery(SQL);

	int tupleNum = PQntuples(res);
	int fieldNum = PQnfields(res);

	for(int i=0;i<fieldNum-1;i++){
		Field += "allroads.";
		char * name = PQfname(res,i);
		if(strstr(name,":") == NULL) 
			Field += name;
		else{
			Field += "\"";
			Field += name;
			Field += "\"";
		}
		Field += ",";
	}
	Field += "ST_AsText(way) as way";
	PQclear(res);
	int id = 1;
	id = Insert("select "+ Field + " from allroads",id);
}

bool readConfig(){
	ifstream fin;
	fin.open("config.ini");
	if(!fin.is_open()){
		cerr <<"can't open config.ini"<<endl;
		return false;
	}
	int cnt = 0;
	string line,prop;
	while(fin >> line){
		if(line == "" || line[0] == '#')
			continue;
		prop = line;
		fin>>line;
		fin>>line;
		cnt++;
		if(prop == "dbname")
			dbname = line;
		else if(prop == "dbport"){
			dbport = line;
		}
		else if(prop == "dbaddr"){
			dbaddr = line;
		}
		else if(prop == "roadTN"){
			roadTN = line;
		}
		else if(prop == "threadnum"){
			int tmp;
			sscanf_s(line.c_str(),"%d",&tmp);
			threadnum = tmp;
		}
		else if(prop == "K"){
			int tmp;
			sscanf_s(line.c_str(),"%d",&tmp);
			K = tmp;
		}
		else if(prop == "top"){
			int tmp;
			sscanf_s(line.c_str(),"%d",&tmp);
			top = tmp;
		}
		else if(prop == "R"){
			double tmp;
			sscanf_s(line.c_str(),"%lf",&tmp);
			R = tmp;
		}
		else if(prop == "sigma"){
			double tmp;
			sscanf_s(line.c_str(),"%lf",&tmp);
			Sigma = tmp;
		}
	}
	fin.close();
	if(cnt == 9) 
		return true;
	else{
		cerr<<"config.ini corrupted��"<<endl;
		return false;
	}
}

int _tmain(int argc, _TCHAR* argv[]){

	//preProcData();
	//cerr<<"over"<<endl;
	if(!readConfig()){
		cerr << "read config.ini error!" <<endl;
		return 0;
	}

	DB = new Database(dbname,dbport,dbaddr);
	network = new Graph(roadTN);

	string basePath;
	cerr<<"please input file path:";
	while(cin>>basePath){
		cerr<<"start init..."<<endl;
		tm = clock();
		if(init(basePath) == false){
			cerr << "can not open file "<< basePath<<endl;
			cerr<<"please input file path:";
			continue;
		}
		cerr<<"init cost "<<clock()-tm<<"ms"<<endl;

		cerr<<"start loadInitPoint..."<<endl;
		tm = clock();
		loadInitPoint();
		cerr<<"loadInitPoint cost "<<clock()-tm<<"ms"<<endl;

		vector <Point> res = ST_Matching_Algorithm();

		cerr<<"start writeToDB->.."<<endl;
		tm = clock();
		writeToDB(res);
		cerr<<"writeToDB cost "<<clock()-tm<<"ms"<<endl;
		cerr<<"please input file path:";
	}
	
	return 0;
}
