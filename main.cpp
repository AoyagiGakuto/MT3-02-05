#include <Novice.h>
#include <cmath>
#include <cstdint>
#include <imgui.h>

const char kWindowTitle[] = "LE2D_02_アオヤギ_ガクト_確認課題";

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Vector3 {
    float x, y, z;
};

struct Matrix4x4 {
    float m[4][4];
};

// 直方体(AABB)
struct Box {
    Vector3 center; // 中心座標
    Vector3 halfSize; // 半分の幅/高さ/奥行き
};

//----------------------------------------
// 行列計算
//----------------------------------------
Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b)
{
    Matrix4x4 r {};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                r.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return r;
}

Matrix4x4 MakeViewProjectionMatrix(const Vector3& cameraTranslate, const Vector3& cameraRotate)
{
    float cosY = cosf(cameraRotate.y);
    float sinY = sinf(cameraRotate.y);
    float cosX = cosf(cameraRotate.x);
    float sinX = sinf(cameraRotate.x);
    float cosZ = cosf(cameraRotate.z);
    float sinZ = sinf(cameraRotate.z);

    Matrix4x4 rotY {};
    rotY.m[0][0] = cosY;
    rotY.m[0][2] = sinY;
    rotY.m[1][1] = 1.0f;
    rotY.m[2][0] = -sinY;
    rotY.m[2][2] = cosY;
    rotY.m[3][3] = 1.0f;

    Matrix4x4 rotX {};
    rotX.m[0][0] = 1.0f;
    rotX.m[1][1] = cosX;
    rotX.m[1][2] = -sinX;
    rotX.m[2][1] = sinX;
    rotX.m[2][2] = cosX;
    rotX.m[3][3] = 1.0f;

    Matrix4x4 rotZ {};
    rotZ.m[0][0] = cosZ;
    rotZ.m[0][1] = -sinZ;
    rotZ.m[1][0] = sinZ;
    rotZ.m[1][1] = cosZ;
    rotZ.m[2][2] = 1.0f;
    rotZ.m[3][3] = 1.0f;

    Matrix4x4 rot = Multiply(Multiply(rotZ, rotX), rotY);

    Matrix4x4 trans {};
    trans.m[0][0] = 1.0f;
    trans.m[1][1] = 1.0f;
    trans.m[2][2] = 1.0f;
    trans.m[3][3] = 1.0f;
    trans.m[3][0] = -cameraTranslate.x;
    trans.m[3][1] = -cameraTranslate.y;
    trans.m[3][2] = -cameraTranslate.z;

    Matrix4x4 view = Multiply(trans, rot);

    Matrix4x4 proj {};
    float fovY = 60.0f * (M_PI / 180.0f);
    float aspect = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float f = 1.0f / tanf(fovY / 2.0f);

    proj.m[0][0] = f / aspect;
    proj.m[1][1] = f;
    proj.m[2][2] = farZ / (farZ - nearZ);
    proj.m[2][3] = (-nearZ * farZ) / (farZ - nearZ);
    proj.m[3][2] = 1.0f;
    proj.m[3][3] = 0.0f;

    return Multiply(view, proj);
}

Matrix4x4 MakeViewportForMatrix(float left, float top, float width, float height, float minDepth, float maxDepth)
{
    Matrix4x4 m {};
    m.m[0][0] = width * 0.5f;
    m.m[1][1] = -height * 0.5f;
    m.m[2][2] = (maxDepth - minDepth);
    m.m[3][0] = left + width * 0.5f;
    m.m[3][1] = top + height * 0.5f;
    m.m[3][2] = minDepth;
    m.m[3][3] = 1.0f;
    return m;
}

Vector3 Transform(const Vector3& v, const Matrix4x4& m)
{
    float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
    float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
    float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
    float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) {
        x /= w;
        y /= w;
        z /= w;
    }
    return { x, y, z };
}

//----------------------------------------
// Grid描画
//----------------------------------------
void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix)
{
    const float kGridHalfWidth = 5.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);

    for (uint32_t xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
        float x = -kGridHalfWidth + xIndex * kGridEvery;
        Vector3 start = { x, 0.0f, -kGridHalfWidth };
        Vector3 end = { x, 0.0f, kGridHalfWidth };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }

    for (uint32_t zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
        float z = -kGridHalfWidth + zIndex * kGridEvery;
        Vector3 start = { -kGridHalfWidth, 0.0f, z };
        Vector3 end = { kGridHalfWidth, 0.0f, z };

        start = Transform(start, viewProjectionMatrix);
        start = Transform(start, viewportMatrix);
        end = Transform(end, viewProjectionMatrix);
        end = Transform(end, viewportMatrix);

        Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0xAAAAAAFF);
    }
}

//----------------------------------------
// 立方体の描画
//----------------------------------------
void DrawBox(const Box& box,
    const Matrix4x4& viewProj,
    const Matrix4x4& viewport,
    uint32_t color)
{

    float hx = box.halfSize.x;
    float hy = box.halfSize.y;
    float hz = box.halfSize.z;

    Vector3 p[8] = {
        { box.center.x - hx, box.center.y - hy, box.center.z - hz },
        { box.center.x + hx, box.center.y - hy, box.center.z - hz },
        { box.center.x + hx, box.center.y + hy, box.center.z - hz },
        { box.center.x - hx, box.center.y + hy, box.center.z - hz },

        { box.center.x - hx, box.center.y - hy, box.center.z + hz },
        { box.center.x + hx, box.center.y - hy, box.center.z + hz },
        { box.center.x + hx, box.center.y + hy, box.center.z + hz },
        { box.center.x - hx, box.center.y + hy, box.center.z + hz }
    };

    for (int i = 0; i < 8; i++) {
        p[i] = Transform(p[i], viewProj);
        p[i] = Transform(p[i], viewport);
    }

    int edges[12][2] = {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, // 下の面
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, // 上の面
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } // 縦の辺
    };

    for (int i = 0; i < 12; i++) {
        Novice::DrawLine((int)p[edges[i][0]].x, (int)p[edges[i][0]].y,
            (int)p[edges[i][1]].x, (int)p[edges[i][1]].y,
            color);
    }
}

//----------------------------------------
// 3D AABB同士の衝突判定
//----------------------------------------
bool IsAABBCollision(const Box& a, const Box& b)
{
    if (fabsf(a.center.x - b.center.x) > (a.halfSize.x + b.halfSize.x))
        return false;
    if (fabsf(a.center.y - b.center.y) > (a.halfSize.y + b.halfSize.y))
        return false;
    if (fabsf(a.center.z - b.center.z) > (a.halfSize.z + b.halfSize.z))
        return false;
    return true;
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    // ライブラリの初期化
    Novice::Initialize(kWindowTitle, 1280, 720);

    // キー入力結果を受け取る箱
    char keys[256] = { 0 };
    char preKeys[256] = { 0 };

    Vector3 cameraTranslate { 0.0f, 12.0f, 40.0f };
    Vector3 cameraRotate { -0.3f, 0.0f, 0.0f };


    // Box A
    Box boxA;
    boxA.center = { -1.5f, 1.0f, 0.0f };
    boxA.halfSize = { 1.0f, 1.0f, 1.0f };

    // Box B
    Box boxB;
    boxB.center = { 2.0f, 1.0f, 0.0f };
    boxB.halfSize = { 1.0f, 1.0f, 1.0f };

    Matrix4x4 viewportMatrix = MakeViewportForMatrix(
        0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 1.0f);
    Matrix4x4 viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始
        Novice::BeginFrame();

        // キー入力を受け取る
        memcpy(preKeys, keys, 256);
        Novice::GetHitKeyStateAll(keys);

        ///
        /// ↓更新処理ここから
        ///

        viewProjectionMatrix = MakeViewProjectionMatrix(cameraTranslate, cameraRotate);

        // 衝突判定
        bool hit = IsAABBCollision(boxA, boxB);

        ///
        /// ↑更新処理ここまで
        ///

        ///
        /// ↓描画処理ここから
        ///

        // ImGuiで操作
        ImGui::Begin("Control");
        ImGui::DragFloat3("CameraTranslate", &cameraTranslate.x, 0.01f);
        ImGui::DragFloat3("CameraRotate", &cameraRotate.x, 0.01f);

        ImGui::DragFloat3("BoxA Pos", &boxA.center.x, 0.05f);
        ImGui::DragFloat3("BoxA HalfSize", &boxA.halfSize.x, 0.05f);

        ImGui::DragFloat3("BoxB Pos", &boxB.center.x, 0.05f);
        ImGui::DragFloat3("BoxB HalfSize", &boxB.halfSize.x, 0.05f);

        ImGui::Text("Collision: %s", hit ? "YES" : "NO");
        ImGui::End();

        // Grid描画
        DrawGrid(viewProjectionMatrix, viewportMatrix);

        // Box A (白 or 赤)
        DrawBox(boxA, viewProjectionMatrix, viewportMatrix, hit ? RED : WHITE);
        // Box B (青 or 赤)
        DrawBox(boxB, viewProjectionMatrix, viewportMatrix, hit ? RED : BLUE);

        ///
        /// ↑描画処理ここまで
        ///

        // フレームの終了
        Novice::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
            break;
        }
    }

    // ライブラリの終了
    Novice::Finalize();
    return 0;
}