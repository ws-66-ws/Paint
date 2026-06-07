// Paint.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "Paint.h"

#define MAX_LOADSTRING 100

#include "framework.h"
#include "Paint.h"
#include <commdlg.h>

#pragma comment(lib, "Comctl32.lib")

#define MAX_LOADSTRING 100

// 图形类型枚举
enum ShapeType {
    SHAPE_FREEHAND,  // 自由画笔
    SHAPE_LINE,      // 直线段
    SHAPE_RECT       // 矩形
};

// 绘图工具枚举
enum DrawTool {
    TOOL_PENCIL,  // 画笔工具
    TOOL_LINE,    // 线段工具
    TOOL_RECT     // 矩形工具
};

// 绘图点结构体
struct MyPoint {
    int x;  // X 坐标
    int y;  // Y 坐标
};

// 图形结构体，用于存储一个完整的图形
struct Shape {
    ShapeType type;              // 图形类型
    COLORREF color;              // 颜色
    int thickness;               // 线宽
    MyPoint start;               // 起点（线段和矩形使用）
    MyPoint end;                 // 终点（线段和矩形使用）
    std::vector<MyPoint> points; // 点集（自由画笔使用）
};

// 所有已绘制的图形
std::vector<Shape> g_shapes;
// 是否正在绘制
bool g_isDrawing = false;
// 当前绘制颜色
COLORREF g_currentColor = RGB(0, 0, 0);
// 当前线宽
int g_currentThickness = 2;
// 当前选中的工具
DrawTool g_currentTool = TOOL_PENCIL;
// 绘制起点（线段和矩形使用）
MyPoint g_startPt = { 0, 0 };
// 是否正在显示橡皮筋预览
bool g_hasPreview = false;
// 预览图形（拖拽时的临时图形）
Shape g_previewShape;
// 工具栏窗口句柄
HWND g_hToolBar = nullptr;

// 绘制所有已完成的图形
// 参数: hdc - 目标设备上下文
void DrawShapes(HDC hdc) {
    for (const auto &shape : g_shapes) {
        // 创建画笔
        HPEN hPen = CreatePen(PS_SOLID, shape.thickness, shape.color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        if (shape.type == SHAPE_FREEHAND) {
            // 自由画笔：沿点集绘制路径
            if (shape.points.empty()) {
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                continue;
            }
            // 移动到起始点
            MoveToEx(hdc, shape.points[0].x, shape.points[0].y, NULL);
            // 依次连接所有点
            for (size_t i = 1; i < shape.points.size(); i++) {
                LineTo(hdc, shape.points[i].x, shape.points[i].y);
            }
        }
        else if (shape.type == SHAPE_LINE) {
            // 线段：从起点到终点画直线
            MoveToEx(hdc, shape.start.x, shape.start.y, NULL);
            LineTo(hdc, shape.end.x, shape.end.y);
        }
        else if (shape.type == SHAPE_RECT) {
            // 矩形：绘制空心矩形
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, shape.start.x, shape.start.y, shape.end.x, shape.end.y);
            SelectObject(hdc, hOldBrush);
        }

        // 恢复旧画笔并删除创建的画笔
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
}

// 绘制橡皮筋预览（拖拽时的虚线预览）
// 参数: hdc - 目标设备上下文
void DrawPreview(HDC hdc) {
    if (!g_hasPreview) return;

    // 创建虚线画笔用于预览
    HPEN hPen = CreatePen(PS_DASH, g_previewShape.thickness, g_previewShape.color);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    if (g_previewShape.type == SHAPE_LINE) {
        // 预览线段
        MoveToEx(hdc, g_previewShape.start.x, g_previewShape.start.y, NULL);
        LineTo(hdc, g_previewShape.end.x, g_previewShape.end.y);
    }
    else if (g_previewShape.type == SHAPE_RECT) {
        // 预览矩形（空心）
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, g_previewShape.start.x, g_previewShape.start.y, g_previewShape.end.x, g_previewShape.end.y);
        SelectObject(hdc, hOldBrush);
    }

    // 恢复旧画笔并删除预览画笔
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PAINT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PAINT));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAINT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PAINT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // 设置格式菜单中默认选中的线宽（2像素）
   CheckMenuRadioItem(GetMenu(hWnd), ID_FORMAT_WIDTH_1, 
       ID_FORMAT_WIDTH_6, ID_FORMAT_WIDTH_2, MF_BYCOMMAND);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // 初始化公共控件
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&icex);

        // 创建工具栏
        g_hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST,
            0, 0, 0, 0,
            hWnd, (HMENU)0, hInst, NULL);

        // 设置工具栏按钮结构大小
        SendMessage(g_hToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

        // 定义工具栏按钮
        TBBUTTON tbb[3] = {};
        // 初始化按钮公共属性
        for (int i = 0; i < 3; i++) {
            tbb[i].iBitmap = I_IMAGENONE;     // 无图标
            tbb[i].fsState = TBSTATE_ENABLED;  // 启用状态
            tbb[i].fsStyle = BTNS_AUTOSIZE | BTNS_CHECKGROUP;  // 自动大小 + 互斥选中
            tbb[i].iString = (INT_PTR)L"";
        }
        // 画笔按钮
        tbb[0].idCommand = ID_TOOL_PENCIL;
        tbb[0].iString = (INT_PTR)L"画笔";
        tbb[0].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;  // 默认选中
        // 线段按钮
        tbb[1].idCommand = ID_TOOL_LINE;
        tbb[1].iString = (INT_PTR)L"线段";
        // 矩形按钮
        tbb[2].idCommand = ID_TOOL_RECT;
        tbb[2].iString = (INT_PTR)L"矩形";

        // 添加按钮到工具栏
        SendMessage(g_hToolBar, TB_ADDBUTTONS, 3, (LPARAM)tbb);
        // 自动调整工具栏大小
        SendMessage(g_hToolBar, TB_AUTOSIZE, 0, 0);
        break;
    }
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
                // 切换到画笔工具
            case ID_TOOL_PENCIL:
                g_currentTool = TOOL_PENCIL;
                SendMessage(g_hToolBar, TB_CHECKBUTTON, ID_TOOL_PENCIL, TRUE);
                break;
                // 切换到线段工具
            case ID_TOOL_LINE:
                g_currentTool = TOOL_LINE;
                SendMessage(g_hToolBar, TB_CHECKBUTTON, ID_TOOL_LINE, TRUE);
                break;
                // 切换到矩形工具
            case ID_TOOL_RECT:
                g_currentTool = TOOL_RECT;
                SendMessage(g_hToolBar, TB_CHECKBUTTON, ID_TOOL_RECT, TRUE);
                break;
                // 撤销上一步操作
            case ID_EDIT_UNDO:
                if (!g_shapes.empty()) {
                    g_shapes.pop_back();
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                break;
                // 清空画布
            case ID_EDIT_CLEAR:
                g_shapes.clear();
                InvalidateRect(hWnd, NULL, TRUE);
                break;
                // 选择颜色
            case ID_FORMAT_COLOR:
            {
                static COLORREF acrCustClr[16] = {};  // 自定义颜色数组
                CHOOSECOLOR cc = {};
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hWnd;
                cc.lpCustColors = acrCustClr;         // 自定义颜色
                cc.rgbResult = g_currentColor;         // 当前颜色作为初始值
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;   // 完全展开 + 使用初始颜色
                if (ChooseColor(&cc)) {
                    // 用户确认选择，更新当前颜色
                    g_currentColor = cc.rgbResult;
                }
                break;
            }
            // 设置线宽为 1 像素
            case ID_FORMAT_WIDTH_1:
                g_currentThickness = 1;
                CheckMenuRadioItem(GetMenu(hWnd), ID_FORMAT_WIDTH_1, ID_FORMAT_WIDTH_6, ID_FORMAT_WIDTH_1, MF_BYCOMMAND);
                break;
                // 设置线宽为 2 像素
            case ID_FORMAT_WIDTH_2:
                g_currentThickness = 2;
                CheckMenuRadioItem(GetMenu(hWnd), ID_FORMAT_WIDTH_1, ID_FORMAT_WIDTH_6, ID_FORMAT_WIDTH_2, MF_BYCOMMAND);
                break;
                // 设置线宽为 4 像素
            case ID_FORMAT_WIDTH_4:
                g_currentThickness = 4;
                CheckMenuRadioItem(GetMenu(hWnd), ID_FORMAT_WIDTH_1, ID_FORMAT_WIDTH_6, ID_FORMAT_WIDTH_4, MF_BYCOMMAND);
                break;
                // 设置线宽为 6 像素
            case ID_FORMAT_WIDTH_6:
                g_currentThickness = 6;
                CheckMenuRadioItem(GetMenu(hWnd), ID_FORMAT_WIDTH_1, ID_FORMAT_WIDTH_6, ID_FORMAT_WIDTH_6, MF_BYCOMMAND);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        // 鼠标左键按下：开始绘制
    case WM_LBUTTONDOWN:
    {
        // 标记开始绘制
        g_isDrawing = true;
        // 获取鼠标坐标
        MyPoint pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        // 记录起点
        g_startPt = pt;

        if (g_currentTool == TOOL_PENCIL) {
            // 画笔工具：创建新的自由画笔图形
            Shape shape = {};
            shape.type = SHAPE_FREEHAND;
            shape.color = g_currentColor;
            shape.thickness = g_currentThickness;
            shape.points.push_back(pt);
            g_shapes.push_back(shape);
        }
        else {
            // 线段/矩形工具：初始化橡皮筋预览
            g_hasPreview = true;
            g_previewShape.type = (g_currentTool == TOOL_LINE) ? SHAPE_LINE : SHAPE_RECT;
            g_previewShape.color = g_currentColor;
            g_previewShape.thickness = g_currentThickness;
            g_previewShape.start = pt;
            g_previewShape.end = pt;
        }
        break;
    }
    // 鼠标移动：更新绘制
    case WM_MOUSEMOVE:
    {
        // 获取鼠标坐标
        MyPoint pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_isDrawing) {
            if (g_currentTool == TOOL_PENCIL && !g_shapes.empty()) {
                // 画笔工具：添加新点到当前图形
                g_shapes.back().points.push_back(pt);
                // 计算需要重绘的区域
                RECT updateRect;
                auto &lastPt = g_shapes.back().points[g_shapes.back().points.size() - 2];
                int offset = g_currentThickness + 2;
                updateRect.left = min(lastPt.x, pt.x) - offset;
                updateRect.top = min(lastPt.y, pt.y) - offset;
                updateRect.right = max(lastPt.x, pt.x) + offset;
                updateRect.bottom = max(lastPt.y, pt.y) + offset;
                // 请求重绘
                InvalidateRect(hWnd, &updateRect, FALSE);
            }
            else if (g_hasPreview) {
                // 线段/矩形工具：更新预览终点
                g_previewShape.end = pt;
                // 计算需要重绘的区域
                RECT updateRect;
                int offset = g_currentThickness + 2;
                updateRect.left = min(g_startPt.x, pt.x) - offset;
                updateRect.top = min(g_startPt.y, pt.y) - offset;
                updateRect.right = max(g_startPt.x, pt.x) + offset;
                updateRect.bottom = max(g_startPt.y, pt.y) + offset;
                InvalidateRect(hWnd, &updateRect, FALSE);
            }
        }
        break;
    }
    // 鼠标左键释放：结束绘制
    case WM_LBUTTONUP:
    {
        if (g_isDrawing) {
            if (g_currentTool != TOOL_PENCIL && g_hasPreview) {
                // 线段/矩形工具：将预览图形提交到图形列表
                Shape shape = g_previewShape;
                g_shapes.push_back(shape);
                g_hasPreview = false;
                // 重绘整个窗口
                InvalidateRect(hWnd, NULL, TRUE);
            }
            // 结束绘制
            g_isDrawing = false;
        }
        break;
    }
    case WM_PAINT:
        {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 获取客户区大小
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        // 创建兼容内存 DC（双缓冲）
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        // 填充白色背景
        HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
        FillRect(memDC, &clientRect, whiteBrush);

        // 绘制所有已完成的图形
        DrawShapes(memDC);
        // 绘制橡皮筋预览
        DrawPreview(memDC);

        // 将内存 DC 内容复制到屏幕
        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        // 清理资源
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
            EndPaint(hWnd, &ps);
        }
        break;
        // 设置光标样式
    case WM_SETCURSOR:
    {
        // 仅在客户区内切换光标
        if (LOWORD(lParam) == HTCLIENT) {
            if (g_currentTool == TOOL_PENCIL) {
                // 画笔工具使用箭头光标
                SetCursor(LoadCursor(NULL, IDC_ARROW));
            }
            else {
                // 线段/矩形工具使用十字光标
                SetCursor(LoadCursor(NULL, IDC_CROSS));
            }
            return TRUE;
        }
        break;
    }
    // 窗口大小改变时自动调整工具栏
    case WM_SIZE:
        SendMessage(g_hToolBar, TB_AUTOSIZE, 0, 0);
        break;
        // 菜单弹出时同步选中状态
    case WM_INITMENUPOPUP:
    {
        // 打开格式菜单时同步线宽选中状态
        if (GetSubMenu(GetMenu(hWnd), 2) == (HMENU)wParam) {
            CheckMenuRadioItem((HMENU)wParam, ID_FORMAT_WIDTH_1, ID_FORMAT_WIDTH_6,
                (g_currentThickness == 1) ? ID_FORMAT_WIDTH_1 :
                (g_currentThickness == 2) ? ID_FORMAT_WIDTH_2 :
                (g_currentThickness == 4) ? ID_FORMAT_WIDTH_4 :
                ID_FORMAT_WIDTH_6, MF_BYCOMMAND);
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
