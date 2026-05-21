### Assigment1注意事项

作业框架传入的ZNear与ZFar是正值，而课上的projection公式是根据ZNear与ZFar为负值推的。

所以直接代入课上推导的矩阵，最后画出来的是一个倒三角形。需要将ZNear与ZFar乘-1。



为什么是倒三角？

个人理解因为作业框架没有做裁剪，因此除了视锥体之内，视锥体之外的对应空间物体也会被投影到视锥体之内。

而当对应图形位于原点另一边(视锥体在正半轴，图形在负半轴)，投影过去就会上下颠倒。

![1](G:\自学作业项目\games101\mySolution\Assignment1\assignment1\1.jpg)