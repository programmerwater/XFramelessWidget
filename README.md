# XFramelessWidget

A frameless widget based on **Qt** and native apis. Support **Windows**, **Linux** and **MacOS**.

## How to Use

## Implementation Summary

### Windows

The trick is that **XFramelessWidget** has a parent of native **Windows**'s window. With
some clever use of **HTTRANSPARENT** to pass events to the parent native **Windows**'s window
for resizing and dragging, and passing close event the other way to **XFramelessWidget** for
handling within **Qt** there.

You can own almost entirely native window features, including Min, Max, Close, Resize and Aero snap.

### Linux

### MacOS

Setting **NSWindow**'s style and appearance to hide the titlebar.

## ToDo

- QTest自动化测试；
- 实现阴影绘制；
- 更多顶层窗口特性支持


## Reference

|No.|Title|Remark|
|:--:|:--:|:--:|
|1|[TrueFramelessWindow](https://github.com/dfct/TrueFramelessWindow)||
|2|[FLWidget](https://github.com/CryFeiFei/FLWidget)||
|3|[Linux Deepin](https://raw.githubusercontent.com/linuxdeepin/dtkwidget/82bbc6fb20b43c17a957b10ebfd586a90a4a909f/src/platforms/x11/xutil.cpp)||
|4|[qtwinmigrate](https://github.com/qtproject/qt-solutions/tree/master/qtwinmigrate/src)||
