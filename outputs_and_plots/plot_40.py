import matplotlib.pyplot as plt
import numpy as np
def get_strs(strs):
    s=[]
    for st in strs:
        if st!="":
            s.append(st)
    return s 

def load_data(filename):
    file=open(filename)
    lines=file.readlines()
    file.close()
    epoch,auc,hit,mapk=[],[],[],[]
    for line in lines:
        strs=line.replace("\t"," ").split(' ')
        strs=get_strs(strs)
        if len(strs)!=4:
            continue
        epoch.append(int(strs[0]))
        auc.append(float(strs[1]))
        hit.append(float(strs[2]))
        mapk.append(float(strs[3]))
    print filename,"len(epoch):",len(epoch),"len(auc):",len(auc),"len(hit):",len(hit),"len(mapk):",len(mapk)
    return [epoch,auc,hit,mapk]

def get_mean(xx):
    xxx=0.0
    for x in xx:
        xxx+=x 
    return xxx/len(xx)
    
p40=load_data('bp40.txt')
t40=load_data('bt40.txt')
epoch=p40[0]if len(p40[0])<len(t40[0]) else t40[0]

max_epoch=190
epoch=epoch[:max_epoch]
py=p40[3][:max_epoch]
ty=t40[3][:max_epoch]

for i in range(65,max_epoch):
    ty[i]=float('nan')

epoch=epoch+[max_epoch]
py=[0.0]+py
ty=[0.0]+ty

xs=np.array(epoch)
sta1=np.array(py)
sta2=np.array(ty)

plt.figure(figsize=(6,4))
plt.plot(xs,sta2,label='BPR+TransR+SDAE',linestyle="-")   #,marker='o'
plt.plot(xs,sta1,label='MFBPR',linestyle="-")   #,marker='^'
#plt.ylim(0.042, 0.049)#sta1.min()*1.1,sta2.max()*1.1
#plt.xticks(xs,xs)
plt.xlabel('epoch')
plt.ylabel('MAP@K')
plt.legend(frameon=False)
plt.show()
