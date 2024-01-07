import casioplot as plt
limits=[17,56,127,0]
fenetre=[0,1,0,1]
points=[[]]
lines=[[]]
textes=[[]]
extrem=[0,1,0,1]
win_scaling='init'
axis_display='on'
grid_display='off'
color_grid='grey'
available_colors=['b','r','g','k','m','c','y','w','blue','red','green','black','magenta','cyan','yellow','white','grey','orange','purple','brown','pink']

def axis(*L):
    global fenetre,win_scaling,axis_display
    if len(L)==1 and L[0]=='auto':
        win_scaling='auto'
    if L==() or L[0]=='auto':
        if win_scaling=='auto':
            for i in [0,2]:
                if extrem[i]==extrem[i+1]:
                    if extrem[i]==0:
                        fenetre[i:i+2]=[-0.05,0.05]
                    else:
                        fenetre[i:i+2]=[extrem[i]-0.05*abs(extrem[i]),extrem[i]+0.05*abs(extrem[i])]
                else:
                    fenetre[i:i+2]=[1.05*extrem[i]-0.05*extrem[i+1],1.05*extrem[i+1]-0.05*extrem[i]]
        return fenetre
    elif isinstance(L[0],(list,tuple)) and len(L[0])==4:
        fenetre=list(L[0])
        if fenetre[0]==fenetre[1]:
            if fenetre[0]==0:
                fenetre[0:2]=[-0.05,0.05]
            else:
                fenetre[0:2]=[0.95*fenetre[0],1.05*fenetre[0]]
            print('Userwarning: attempting to set identical bottom == top in function axis(); automatically expanding.')
        if fenetre[2]==fenetre[3]:
            if fenetre[2]==0:
                fenetre[2:4]=[-0.05,0.05]
            else:
                fenetre[2:4]=[0.95*fenetre[2],1.05*fenetre[2]]
            print('Userwarning: attempting to set identical bottom == top in function axis(); automatically expanding.')
        win_scaling='fixed'
        axis_display='on'
        return fenetre
    elif L[0]=='off':
        axis_display='off'
    elif L[0]=='on':
        axis_display='on'
    else:
        raise Exception('function axis() : error using arguments')

def text(x,y,txt):
    global textes
    txt=str(txt)
    if textes==[[]]:
        textes[0]=[x,y,txt]
    else:
        if [x,y,txt] not in textes :
            textes+=[[x,y,txt]]

def plot(*L,**kwargs):
    global win_scaling
    def plotpoint(x,y):
        global points,extrem,win_scaling
        if points==[[]] and lines==[[]]:
            points[0]=[x,y]
            extrem=[x,x,y,y]
        else:
            if [x,y] not in points :
                points+=[[x,y]]
                extrem=[min(x,extrem[0]),max(x,extrem[1]),min(y,extrem[2]),max(y,extrem[3])]
        if win_scaling=='init':
            win_scaling='auto'

    def plotline(x1,y1,x2,y2):
        global lines,extrem,win_scaling
        if lines==[[]] and points==[[]]:
            lines[0]=[x1,y1,x2,y2]
            extrem=[min(x1,x2),max(x1,x2),min(y1,y2),max(y1,y2)]
        else:
            if [x1,y1,x2,y2] not in lines :
                lines+=[[x1,y1,x2,y2]]
                extrem=[min(x1,x2,extrem[0]),max(x1,x2,extrem[1]),min(y1,y2,extrem[2]),max(y1,y2,extrem[3])]
        if win_scaling=='init':
            win_scaling='auto'

    def testcolor(color):
        global available_colors
        if (len(color)==2 and color[0] in available_colors and color[1] in ['o','.','+','*','-']) or (color in available_colors+['o','.','+','*','-']) :
            return True
        else:
            raise ValueError('function plot() : unknown color code')

    color=kwargs.get('color','k')
    if color!=None and not color in available_colors:
        raise ValueError('function plot() : unknown color code')
    if len(L)==2 and isinstance(L[0],(int,float)) and isinstance(L[1],(int,float)):
        plotpoint(L[0],L[1])
    elif len(L)==2 and isinstance(L[0],(list,tuple)) and isinstance(L[1],(list,tuple)):
        if (len(L[0])==len(L[1])):
            for i in range(len(L[0])-1):
                plotline(L[0][i],L[1][i],L[0][i+1],L[1][i+1])
        else:
            raise ValueError('function plot() : x and y must have same dimension')
    elif len(L)==1 and isinstance(L[0],(list,tuple)):
        for i in range(len(L[0])-1):
            plotline(i,L[0][i],i+1,L[0][i+1])
    elif len(L)==3 and isinstance(L[0],(int,float)) and isinstance(L[1],(int,float)) and isinstance(L[2],(str)):
        if testcolor(L[2])==True :
            plotpoint(L[0],L[1])
    elif len(L)==3 and isinstance(L[0],(list,tuple)) and isinstance(L[1],(list,tuple)) and isinstance(L[2],(str)):
        if (len(L[0])==len(L[1])):
            if testcolor(L[2])==True :
                for i in range(len(L[0])-1):
                    plotline(L[0][i],L[1][i],L[0][i+1],L[1][i+1])
        else:
            raise ValueError('function plot() : x and y must have same dimension')
    elif len(L)==2 and isinstance(L[0],(list,tuple)) and isinstance(L[1],(str)):
        color=L[1]
        if testcolor(L[1])==True :
            for i in range(len(L[0])-1):
                plotline(i,L[0][i],i+1,L[0][i+1])
    else:
        raise Exception('function plot() : error using arguments')
def show():
    global fenetre, limits, points, lines, textes, extrem, win_scaling, axis_display, grid_display
    def printable(X):
        global limits
        return(limits[0]<=X[0]<=limits[2] and limits[3]<=X[1]<=limits[1])
    def echelle(a,b):
        k=0
        e=abs(b-a)
        while e>=10 :
            e/=10
            k+=1
        while e<1 :
            e*=10
            k-=1
        return k
    def pas(a,b):
        pas=10**echelle(a,b)
        while (abs(b-a))//pas<3:
            pas/=2
        return pas
    def converttopixel(X):
        global fenetre,limits
        ax=(limits[2]-limits[0])/(fenetre[1]-fenetre[0])
        bx=limits[0]-ax*fenetre[0]
        xpixel=round(ax*X[0]+bx)
        ay=(limits[3]-limits[1])/(fenetre[3]-fenetre[2])
        by=limits[1]-ay*fenetre[2]
        ypixel=round(ay*X[1]+by)
        return [xpixel,ypixel]

    plt.clear_screen()
    if win_scaling=='auto':
        for i in [0,2]:
            if extrem[i]==extrem[i+1]:
                if extrem[i]==0:
                    fenetre[i:i+2]=[-0.05,0.05]
                else:
                    fenetre[i:i+2]=[extrem[i]-0.05*abs(extrem[i]),extrem[i]+0.05*abs(extrem[i])]
            else:
                fenetre[i:i+2]=[1.05*extrem[i]-0.05*extrem[i+1],1.05*extrem[i+1]-0.05*extrem[i]]
    if axis_display=='on' or axis_display=='boxplot':
        for i in range(limits[0],limits[2]+1):
            plt.set_pixel(i,limits[1],(1,1,1))
        for j in range(limits[3],limits[1]+1):
            plt.set_pixel(limits[0],j,(1,1,1))
        fenetreb=sorted([fenetre[0],fenetre[1]])+sorted([fenetre[2],fenetre[3]])
        pasx=pas(fenetreb[0],fenetreb[1])
        pasy=pas(fenetreb[2],fenetreb[3])
        gx=round(fenetreb[0],-echelle(fenetreb[0],fenetreb[1]))
        gy=round(fenetreb[2],-echelle(fenetreb[2],fenetreb[3]))
        if axis_display=='boxplot':
            for i in range(nbre_boite):
                pix=converttopixel((i+1,fenetre[2]))
                plt.set_pixel(pix[0],pix[1]+1,(1,1,1))
                plt.draw_string(pix[0],pix[1]+3,str(i+1),[0,0,0],"small")
        else :
            for i in range(-10,10):
                x=gx+i*pasx
                pix=converttopixel((x,fenetre[2]))
                if printable(pix):
                    plt.set_pixel(pix[0],pix[1]+1,(1,1,1))
                    if grid_display=='on':
                        for z in range(1,165,3):
                            plt.set_pixel(pix[0],pix[1]-z,(1,1,1))
                    plt.draw_string(pix[0],pix[1]+3,'{:.4g}'.format(x),[0,0,0],"small")
        for j in range(-10,10):
            y=gy+j*pasy
            pix=converttopixel((fenetre[0],y))
            if printable(pix):
                plt.set_pixel(pix[0]-1,pix[1],(1,1,1))
                if grid_display=='on':
                    for z in range(1,340,3):
                        plt.set_pixel(pix[0]+z,pix[1],(1,1,1))
                plt.draw_string(pix[0]-17,pix[1],'{:.4g}'.format(y),[0,0,0],"small")
    if points!=[[]]:
        if points[0]==[]:
            del points[0]
        for i in range(len(points)):
            pix=converttopixel((points[i][0],points[i][1]))
            if printable(pix):
                for j in range(-1,2):
                    plt.set_pixel(pix[0]+j,pix[1],(1,1,1))
                    plt.set_pixel(pix[0],pix[1]+j,(1,1,1))
    if textes!=[[]]:
        if textes[0]==[]:
            del textes[0]
        for i in range(len(textes)):
            pix=converttopixel((textes[i][0],textes[i][1]))
            if printable(pix):
                plt.draw_string(pix[0],pix[1],textes[i][2],[0,0,0],"small")
    if lines!=[[]]:
        if lines[0]==[]:
            del lines[0]
        for i in range(len(lines)):
            pixels=[converttopixel((lines[i][j],lines[i][j+1])) for j in [0,2]]
            deltax=abs(pixels[1][0]-pixels[0][0])
            deltay=abs(pixels[1][1]-pixels[0][1])
            if deltax<=1 and deltay<=1:
                if printable((pixels[0][0],pixels[0][1])):
                    plt.set_pixel(pixels[0][0],pixels[0][1],(1,1,1))
                    plt.set_pixel(pixels[1][0],pixels[1][1],(1,1,1))
            else:
                if deltax>=deltay:
                    j=0
                else:
                    j=1
                m=(pixels[1][1-j]-pixels[0][1-j])/(pixels[1][j]-pixels[0][j])
                p=pixels[0][1-j]-m*pixels[0][j]
                pix_extrem=(max(limits[0],min(pixels[0][0],pixels[1][0])),min(limits[2],max(pixels[0][0],pixels[1][0])),max(limits[3],min(pixels[0][1],pixels[1][1])),min(limits[1],max(pixels[0][1],pixels[1][1])))
                if pix_extrem[2*j]<=limits[2-j] and pix_extrem[2*j+1]>=limits[-j]:
                    pix=[0,0]
                    for pix[j] in range(pix_extrem[2*j],pix_extrem[2*j+1]+1):
                        pix[1-j]=round(m*pix[j]+p)
                        if printable(pix):
                            plt.set_pixel(pix[0],pix[1],(1,1,1))
    axis([limits[0]-50,limits[2],limits[1]+50,limits[3]])
    axis("off")
    plt.show_screen()
    points=[[]]
    lines=[[]]
    textes=[[]]
    extrem=[0,1,0,1]
    fenetre=[0,1,0,1]
    axis_display='on'
    win_scaling='init'
    grid_display='off'

def bar(val,eff,width=0.8):
    if isinstance(val,(tuple)):
        val=list(val)
    if isinstance(eff,(tuple)):
        eff=list(eff)
    if isinstance(val,(int,float)):
        val=[val]
    if isinstance(eff,(int,float)):
        eff=[eff]
    if isinstance(val,(list)) and isinstance(eff,(list)):
        if len(val)==len(eff):
            for i in range(len(val)):
                plot([val[i]-width/2,val[i]-width/2],[0,eff[i]],'k')
                plot([val[i]+width/2,val[i]+width/2],[0,eff[i]],'k')
                plot([val[i]-width/2,val[i]+width/2],[eff[i],eff[i]],'k')
        else:
            raise ValueError('function bar() : lists must have same dimension')
    else:
        raise ValueError('function bar() : error using arguments')

def scatter(xlist,ylist):
    if isinstance(xlist,(tuple)):
        xlist=list(xlist)
    if isinstance(ylist,(tuple)):
        ylist=list(ylist)
    if isinstance(xlist,(int,float)):
        xlist=[xlist]
    if isinstance(ylist,(int,float)):
        ylist=[ylist]
    if isinstance(xlist,(list)) and isinstance(ylist,(list)):
        if len(xlist)==len(ylist):
            for i in range(len(xlist)):
                plot(xlist[i],ylist[i],'k')
        else:
            raise ValueError('function scatter() : x and y lists must have same dimension')
    else:
        raise ValueError('function scatter() : error using arguments')

def hist(x,bins=10,**kwargs):
    hist_type=kwargs.get('hist_type','std')
    if hist_type not in ['fr','std']:
        raise ValueError('function hist() : hist_type must be std or fr')
    if isinstance(x,(tuple,list)):
        x=sorted(list(x))
    if isinstance(bins,(tuple)):
        bins=list(bins)
    if isinstance(x,(int,float)):
        x=[x]
    if isinstance(bins,(int)) and bins>=1:
        if x[-1]!=x[0]:
            bins=[round(x[0]+k*(x[-1]-x[0])/bins,8) for k in range(bins+1)]
        else :
            bins=[round(x[0]-0.5+k/bins,8) for k in range(bins+1)]
    if isinstance(bins,(list)) and bins!=[]:
        bins=sorted(bins)
        qt=[]
        for i in range(len(bins)-1):
            if i==len(bins)-2:
                eff=len([val for val in x if bins[i]<=val<=bins[i+1]])
            else:
                eff=len([val for val in x if bins[i]<=val<bins[i+1]])
            if hist_type=='fr':
                if abs(bins[i+1]-bins[i])>1e-8:
                    eff=eff/(bins[i+1]-bins[i])
                else :
                    raise ValueError('function hist(,hist_type=''fr'') : bins cannot contain 2 identical values')
            qt+=[eff]
            plot([bins[i],bins[i],bins[i+1],bins[i+1]],[0,eff,eff,0],'k')
    else:
        raise ValueError('function hist() : error using arguments')
    return qt,bins

def boxplot(L,**kwargs):
    L=list(L)
    global fenetre,nbre_boite,axis_display,win_scaling
    boxplot_type=kwargs.get('boxplot_type','std')
    if boxplot_type not in ['fr','std']:
        raise ValueError('function boxplot() : boxplot_type must be std or fr')
    def mediane(l,p):
        if p%2==0:
            return (l[p//2]+l[p//2-1])/2
        else:
            return l[p//2]
    def quantiles(l,p,r):
        if boxplot_type=='fr':
            if p%r==0:
                return (l[p//r-1],l[((r-1)*p)//r-1])
            else:
                return (l[p//r],l[((r-1)*p)//r])
        if boxplot_type=='std':
            def percentile(N, q):
                k = (len(N)-1) * q
                f = int(k)
                c = int(k)+1
                if f == k:
                    return N[int(k)]
                d0 = N[f] * (c-k)
                d1 = N[c] * (k-f)
                return d0+d1
            return (percentile(l,0.25),percentile(l,0.75))
    whis=kwargs.get('whis',1.5)
    if whis<0:
        whis=0
    axis_display='boxplot'
    n=len(L)
    if isinstance(L[0],(int,float)):
        n=1
        Max,Min=max(L),min(L)
        if  Max==Min:
            ampl=1
        else:
            ampl=Max-Min
        fenetre=[0,2,Min-ampl/20,Max+ampl/20]
    else:
        Max,Min=max([max(L[i]) for i in range(len(L))]),min([min(L[i]) for i in range(len(L))])
        if  Min==Max:
            ampl=1
        else:
            ampl=Max-Min
        fenetre=[0,len(L)+1,Min-ampl/20,Max+ampl/20]
    nbre_boite,largeur=n,0.3/n
    win_scaling='fixed'
    for i in range(n):
        if n==1:
            if isinstance(L[0],(int,float)):
                K=L
            else:
                K=L[0]
        else :
            K=L[i]
        if isinstance(K,(int,float)):
            plot([i+1-largeur,i+1+largeur],[K,K],'r')
        elif isinstance(K[0],(int,float)):
            K,p=sorted(K),len(K)
            med=mediane(K,p)
            Q1,Q3=quantiles(K,p,4)
            if boxplot_type=='std':
                down,up=0,p-1
                while Q1-whis*(Q3-Q1)>K[down]:
                    down+=1
                while Q3+whis*(Q3-Q1)<K[up]:
                    up-=1
                left_whis,right_whis=K[down],K[up]
                if Q1<K[down]:
                    left_whis=Q1
                if Q3>K[up]:
                    right_whis=Q3
            if boxplot_type=='fr':
                D1,D9=quantiles(K,p,10)
                down=K.index(D1)
                up=K.index(D9)
                left_whis,right_whis=K[down],K[up]
                while(up<p-1 and K[up]==K[up+1]):
                  up=up+1
                  right_whis=K[up]
            plot([i+1-largeur,i+1+largeur,i+1+largeur,i+1-largeur,i+1-largeur],[Q1,Q1,Q3,Q3,Q1],'k')
            plot([i+1,i+1],[Q1,left_whis],'k')
            plot([i+1,i+1],[Q3,right_whis],'k')
            plot([i+1-largeur/2,i+1+largeur/2],[left_whis,left_whis],'k')
            plot([i+1-largeur/2,i+1+largeur/2],[right_whis,right_whis],'k')
            if down>0 or up<p-1:
                from math import pi,cos,sin
                Z=[i for i in range(down)]+[j for j in range(up+1,p)]
                for t in Z:
                    x=[i+1+0.05*(fenetre[1])/3*cos(2*j*pi/50) for j in range(50)]
                    y=[K[t]+0.05*(fenetre[3]-fenetre[2])/3*sin(2*j*pi/50) for j in range(50)]
                    plot(x,y,'k')
            plot([i+1-largeur,i+1+largeur],[med,med],'r')
        else:
            raise ValueError('wrong type of argument')

def arrow(x,y,dx,dy,**kwargs):
    global win_scaling
    a,b=x+dx,y+dy
    win_scaling='fixed'
    color=kwargs.get('ec','k')
    color=kwargs.get('edgecolor',color)
    L=kwargs.get('head_width',0.003)
    l=kwargs.get('head_length',1.5*L)
    plot((x,a),(y,b),color)

    def resol(A,B,C):
        D=B**2-4*A*C
        if D>0:
            return((-B-D**0.5)/(2*A),(-B+D**0.5)/(2*A))
    if abs(dx)<0.00001:
        dx=0
    if dx==0:
        if dy>=0:
            c=1
        else:
            c=-1
        plot((a-L/2,a+L/2,a,a-L/2),(b,b,b+c*l,b),color)
        
    elif dy==0:
        if dx>=0:
            c=1
        else:
            c=-1
        plot((a,a,a+c*l,a),(b-L/2,b+L/2,b,b-L/2),color)
        
    else:
        m=dy/dx
        p=y-m*x
        S=resol(m**2+1,2*(-a-b*m+m*p),p**2+a**2+b**2-l**2-2*b*p)
        if (a-S[0])*dx<0:
            X=S[0]
        else:
            X=S[1]
        Y=m*X+p
        k=b+a/m
        T=resol(1+1/m**2,2*(-a-k/m+b/m),a**2+k**2-2*b*k+b**2-(L**2)/4)
        plot((T[0],T[1],X,T[0]),(-T[0]/m+k,-T[1]/m+k,Y,-T[0]/m+k),color)
        

def grid(*a,**kwargs):
    global color_grid,grid_display
    color=kwargs.get('color',None)
    if a==():
        affichage=a
    else:
        affichage=a[0]
    if color!=None:
        color_grid=color
        affichage='on'
    if color==None:
        color_grid='k'
    if not color in available_colors:
        color_grid='grey'
    if affichage==():
        if grid_display=='on':
            grid_display='off'
        else:
            grid_display='on'
    elif affichage=='on' or affichage=='True' or affichage==True or affichage=='true':
        grid_display='on'
    elif affichage=='off' or affichage=='False' or affichage==False or affichage=='false':
        grid_display='off'
    else:
        raise ValueError('string must be one of : "on","off","True",or "False"')
