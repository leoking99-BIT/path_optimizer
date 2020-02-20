# path_optimizer [移动到这里了](https://github.com/LiJiangnanBit/path_optimizer/tree/master)  
根据参考路径（点）生成可行路径。
![cover.png](https://i.loli.net/2020/02/11/CiZXwjQeGNaqdsr.png)  

### Demo 1：手动选择参考点
#### 运行
~~~
catkin build path_optimizer
roslaunch path_optimizer demo.launch
~~~
#### 使用Publish Point工具选择参考路径点 
如果想重新选择，在使用Publish Point工具的情况下任意位置双击即可。   
![ref.gif](https://i.loli.net/2020/02/13/EXB8Qh9MdUOlm1R.gif)  
#### 使用2D Pose Estimate和2D Nav Goal工具选择起点和终点
在目前设置中不强制完全到达终点。  
![calsulate.gif](https://i.loli.net/2020/02/13/mLxIkj4Kvirg7eO.gif)
### Benchmark效率测试  

```
rosrun path_optimizer path_optimizer_benchmark
``` 

## 原理
1. Take inputs (red dots):
![2020-02-13 16-27-46屏幕截图.png](https://i.loli.net/2020/02/13/rRdA7ZGmjfObzNV.png)  
2. (Optional) Use B spline curve fitting to make the path continuous and then search around it for a more reasonable reference path (yellow dots). 
This step can be skipped by changing settings. 
![2020-02-13 16-27-56屏幕截图.png](https://i.loli.net/2020/02/13/GJEbrUIXwScKmWT.png)    
3. Smooth the reference path using IPOPT (yellow curve).   
![2020-02-13 16-28-05屏幕截图.png](https://i.loli.net/2020/02/13/Meqi3m7CXzZFIxJ.png)  
4. Represent the path planning problem as a QP and solve it using OSQP.  
![2020-02-13 16-28-19屏幕截图.png](https://i.loli.net/2020/02/13/HaMpYKcZLxTdtAs.png)
