import matplotlib.pyplot as plt
import numpy as np
def remove_(sts):
    nst=[]
    for s in sts:
        if s==' 'or s=='\t' or s=='\n'or s=='':
            continue 
        #print s
        nst.append(s)
    return nst
    
def get_mean(xx):
    if not xx:
        return 0.0
    xxx=0.0
    for x in xx:
        xxx+=x 
    return xxx/len(xx)
    
f=open('final.txt','r')
temp=[]
stas=[]
for line in f:
    strs=line.split(' ')
    strs=remove_(strs)
    #print len(strs)
    if len(strs)==4:
        temp.append(float(strs[3]))
    else:
        stas.append(get_mean(temp))
        temp=[]
stas.append(get_mean(temp))
temp=[]
stas=stas[1:]
sta1=stas[:len(stas)/2]
sta2=stas[len(stas)/2:]
print len(sta1)
xs=[20+i*20 for i in range(len(sta1))]
xs=np.array(xs)
sta1=np.array(sta1)
sta2=np.array(sta2)
plt.figure(figsize=(6,4))

plt.plot(xs,sta2,label='BPR+TransR+SDAE',linestyle="-",marker='o')   
plt.plot(xs,sta1,label='MFBPR',linestyle="-",marker='^')   
plt.ylim(0.042, 0.049)#sta1.min()*1.1,sta2.max()*1.1
plt.xticks(xs,xs)
plt.xlabel('K')
plt.ylabel('MAP@K')
plt.legend(frameon=False)
plt.show()

f.close()