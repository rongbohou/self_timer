Track algorithm：
1)获得肤色的直方图模型（Hue-Saturation histogram model）;
2）利用反向投影计算待测图像（image），获得 Mat BackProjection;
3）用帧差法获得Mat Diff;
4）按照一定方法，加权BackProjection和Diff，获得Mat probability;
3)利用meanshift计算probability的max trackerbox。