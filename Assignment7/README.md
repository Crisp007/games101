### 1.spp512效果：

![binary_512](images\binary_512.png)

### 2.多线程：

为了不发生数据冲突，按照渲染一张图片流程的特性，直接对图片上不同像素区域进行多线程处理

(每个像素处理逻辑相同，理论上cpu核心越多，能并行的线程越多越快。此代码中的虚拟机只给了4个核的资源，因此将图片分为4个大块进行处理。提速效果明显几乎是四倍提速)

### 3.Mircrofacet微表面效果：

粗糙表面alpha=0.9，ior=1.1（spp值低，噪点较多）

![a0.9_ior1.1](G:\自学作业项目\games101\mySolution\Assignment7\images\a0.9_ior1.1.png)

光滑表面alpha=0.01，ior=16效果（spp=512）：

![a0.01_ior16](G:\自学作业项目\games101\mySolution\Assignment7\images\a0.01_ior16.png)

### 4.注意事项

a.判定 Bounding Box 与 Ray 相交时的边界是 t_enter <= t_exit

b.黑色条纹噪声大概率为精度问题程序误判为shadow场景直接黑了，EPSILON放大到1e-4提升明显

c.get_random_float()随机数生成函数三个变量的定义前面加上static关键字提速

(以上三点作业贴中有提醒)

d.光源区域为纯黑问题：当计算出来的intersection本身就是光源时，就不用再sample light或者reflect了。直接返回intersect.m->getEmission()

e.不同函数对于不同的参数向量方向有不同定义

eval函数计算brdf时，默认radiance的方向都是从交点往外指(与法向量成锐角)

fresnel函数计算菲涅尔项时，I是incident ray由外指向交点(与法向量成钝角)

