#include <iostream>
#include <algorithm>    // 用于std::swap等算法操作
#include <vector>       // 可能需要用于节点操作
#include <queue>        // 可能需要用于层序遍历
#include <memory>       // 若使用智能指针管理内存
#include <stdexcept>  
#include <random>  // 添加头文件支持 std::shuffle

using namespace std;

struct ValidationResult {
    bool isValid;
    string errorMessage;
    int leafLevel;

    ValidationResult(bool valid = true, const string& msg = "", int level = -1)
        : isValid(valid), errorMessage(msg), leafLevel(level) {}
};



/*
m 阶 B 树
    1. 每个节点最多有 m 个子节点  -> m-1 个关键字。
    2. 每个节点至少有 \lceil m/2 \rceil 个子节点。
    3. 根节点至少有两个子节点（除非它是叶子节点）。
    4. 所有叶子节点都在同一层。
    5. 内部节点的关键字按升序排列。
*/
class BTreeNode {
    int n;          // 关键字数量
    BTreeNode **C;  // 子节点数组
    int *keys;      // 关键字数组

    int t;          // 节点最小子节点数
    int m;          // 节点最大关键字数
    bool leaf;      // 是否为叶子节点
public:
    BTreeNode(int _n, bool isleaf);  // 构造函数
    BTreeNode *search(int k);       // 关键字搜索
    void traverse();                // 树遍历
    void insertNonFull(int k);      // 非满节点插入
    void splitChild(int i, BTreeNode *y);  // 子节点分裂
    friend class BTree;
};

// B树类
class BTree {
    BTreeNode *root;  // 根节点
    int m;            // m 阶 B树
public:
    BTree(int _n) {   // 初始化
        root = NULL;
        m = _n;
    }
    void traverse() {  // 遍历入口
        if (root != NULL) root->traverse();
    }
    BTreeNode* search(int k) {  // 查找入口
        return (root == NULL) ? NULL : root->search(k);
    }
    void insert(int k);  // 插入入口
    
    void printTree() {
        if (root == nullptr) {
            cout << "B-Tree is empty." << endl;
            return;
        }

        queue<BTreeNode*> q;
        q.push(root);
        int level = 0;

        while (!q.empty()) {
            cout << "Level " << level << ": [ ";
            int size = q.size();
            for (int i = 0; i < size; i++) { // 提取每个节点子节点
                BTreeNode* node = q.front();
                q.pop();
                
                // 打印当前节点关键字
                for (int j = 0; j < node->n; j++) {
                    cout << node->keys[j];
                    if (j < node->n - 1) cout << ",";
                }
                
                // 将子节点加入队列
                if (!node->leaf) {
                    for (int j = 0; j <= node->n; j++) {
                        if (node->C[j] != nullptr) q.push(node->C[j]);
                    }
                }
                
                // 节点分隔符
                if (i < size - 1) cout << " | ";
            }
            cout << " ]" << endl;
            level++;
        }
    }


    bool validate() {
        if (!root) {
            cout << "✅ B-Tree 验证成功：空树合法。" << endl;
            return true;
        }
        ValidationResult result = validateNode(root, 0, true);
        if (result.isValid) {
            cout << "✅ B-Tree 验证成功：结构合法。" << endl;
        } else {
            cout << "❌ 验证失败：" << result.errorMessage << endl;
        }
        return result.isValid;
    }

private:
    ValidationResult validateNode(BTreeNode* node, int depth, bool isRoot) {
        if (!node) return ValidationResult(false, "节点为 null", depth);

        int minKeys = isRoot ? 1 : ((m + 1) / 2 - 1);
        int maxKeys = m - 1;

        // 检查关键字个数
        if (node->n < minKeys || node->n > maxKeys) {
            return ValidationResult(false,
                "层级 " + to_string(depth) + " 的节点关键字数量不合法，当前为 " + to_string(node->n) +
                "，应在 [" + to_string(minKeys) + ", " + to_string(maxKeys) + "] 内，节点内容: " + keysToStr(node),
                depth);
        }

        // 检查关键字顺序
        for (int i = 1; i < node->n; ++i) {
            if (node->keys[i - 1] >= node->keys[i]) {
                return ValidationResult(false,
                    "层级 " + to_string(depth) + " 的节点关键字未升序排列，节点内容: " + keysToStr(node),
                    depth);
            }
        }

        if (node->leaf) {
            return ValidationResult(true, "", depth);
        }

        // 子节点检查
        if (node->leaf == false) {
            for (int i = 0; i <= node->n; ++i) {
                if (!node->C[i]) {
                    return ValidationResult(false,
                        "非叶子节点在第 " + to_string(depth) + " 层，存在 null 子节点，关键字内容: " + keysToStr(node),
                        depth);
                }
            }
        }

        // 验证子节点
        int expectedLeafLevel = -1;
        for (int i = 0; i <= node->n; ++i) {
            ValidationResult childResult = validateNode(node->C[i], depth + 1, false);
            if (!childResult.isValid) return childResult;

            if (expectedLeafLevel == -1)
                expectedLeafLevel = childResult.leafLevel;
            else if (expectedLeafLevel != childResult.leafLevel) {
                return ValidationResult(false,
                    "层级不一致：不同叶子节点不在同一层。" +
                    to_string(expectedLeafLevel) + " vs " + to_string(childResult.leafLevel),
                    depth);
            }
        }

        return ValidationResult(true, "", expectedLeafLevel);
    }

    string keysToStr(BTreeNode* node) {
        string s = "[ ";
        for (int i = 0; i < node->n; ++i) {
            s += to_string(node->keys[i]);
            if (i < node->n - 1) s += ", ";
        }
        s += " ]";
        return s;
    }
};

/**
 * @brief 构造 B 树节点（设置阶数与叶子属性）
 * 
 * 此构造函数负责：
 * - 设置最小子节点数 t
 * - 设置最大关键字数 m
 * - 标记是否为叶子节点 leaf
 *
 * @param _n 阶数（m 阶 B 树）
 * @param isleaf 是否为叶子节点
 */
BTreeNode::BTreeNode(int _n, bool isleaf) {
    /*
        性质 2：每个节点至少有 \lceil m/2 \rceil 个子节点
    */
    m = _n;
    t = (m + 1) /2; 
    leaf = isleaf;
    
    n = 0;  // 初始化关键字数量为0

    // 性质 1： 每个节点最多有 m-1 个关键字
    C = new BTreeNode*[m + 1];   // 最多 m 个子节点
    keys = new int[m];   // -> 最多 m - 1 个关键字(预留一位溢出，分裂)
    
    // 初始化子节点指针为nullptr
    for (int i = 0; i < m + 1; i++) {
        C[i] = nullptr;  // 修复野指针问题
    }
}

// 关键字搜索
BTreeNode *BTreeNode::search(int k) {
    int i = 0;
    while (i < n && k > keys[i])
        i++;
    if (keys[i] == k)
        return this;
    if (leaf == true)
        return NULL;
    return C[i]->search(k);
}

// 中序遍历
void BTreeNode::traverse() {
    int i;
    for (i = 0; i < n; i++) {
        if (leaf == false)
            C[i]->traverse();
        cout << " " << keys[i];
    }
    if (leaf == false)
        C[i]->traverse();
}

/*
1. 定位终端结点（查找失败为止）
2. 插入关键字
    a. 个数 < `m` →直接插入
    b. 若插入后个数 > `m-1` → 分裂
*/
void BTree::insert(int k) {
    // 初始化根节点
    if (root == NULL) {
        root = new BTreeNode(m, true);
        root->keys[0] = k;
        root->n = 1;
    } else {
        /*
            当根节点插入新节点已满
            -> 则对B-树进行生长操作(分裂)
        */
        root->insertNonFull(k);

        if (root->n == m) { 
            // 为新的根节点分配空间
            BTreeNode *s = new BTreeNode(m, false);
            
            // 将旧的根节点作为新的根节点的孩子
            s->C[0] = root;

            // 将旧的根节点分裂为两个，并将一个元素上移到新的根节点
            s->splitChild(0, root);

            // 新的根节点更新为 s
            root = s;
        }
    }
}


// 非满节点插入
void BTreeNode::insertNonFull(int k) {
    int i = n - 1; // 实际元素尾索引

    // 找到叶子节点插入
    if (leaf == true) { 
        // 找 key[] <= k 最小位置插入  
        while (i >= 0 && keys[i] > k) {
            keys[i+1] = keys[i];
            i--;
        }
        keys[i+1] = k;
        n = n+1;
    } else {
        // 定位 新值 在当前节点 插入位置（后：实际元素尾索引 -> 前 ）
        while (i >= 0 && keys[i] > k){
            i--;
        }

        C[i+1]->insertNonFull(k);
        // 若当前节点 子节点插入后已满 -> 分裂
        if (C[i+1]->n > m - 1) {
            splitChild(i+1, C[i+1]);
        }
    }
}

/**
 * @brief 分裂指定子节点，将其拆分为两个节点并将中间关键字提升到父节点(当前节点)
 * 
 * 具体操作为将子节点 y 从中间位置分裂
 *  [0, t-2] [t-1] [t, n-1]
 *  左半部分保留在原节点 y 中，
 *  右半部分移至新节点 z 中
 *  中间关键字提升到当前节点。
 * 
 * 注：$\lceil (m/2) \rceil$ = t
 * 
 * @param i 当前节点子节点数组中，待分裂子节点 y 所在的索引
 * @param y 指向待分裂的满子节点的指针（原节点）
 */
void BTreeNode::splitChild(int i, BTreeNode *y) {
    BTreeNode *z = new BTreeNode(y->m, y->leaf); // 新节点
    
    // 原节点 y 右半部分 [t, n-1] 移至新节点 z 中
    // 注意： n 为原节点的关键字数量，t 为最小子节点数
    z->n = y->n - t;
    for (int j = 0; j < z->n; j++){
        z->keys[j] = y->keys[j+t]; 
    }
    // 如果 y 不是叶子节点，将右半部分的子节点也移至新节点 z 中
    if (y->leaf == false) {
        // 注意！-> 子节点 = 关键字 + 1
        for (int j = 0; j < z->n + 1; j++){
            z->C[j] = y->C[j+t];
        }
    }
    
    // 左半部分 [0, t-2] 保留在原节点 y 中
    y->n = t-1;

    for (int j = n; j >= i+1; j--){
        C[j+1] = C[j];  
    }
    C[i+1] = z; // 新 z 肯定比 y 大，故放在 y 后面(y: i, z: i+1)
    
    
    for (int j = n-1; j >= i; j--){
        keys[j+1] = keys[j];
    }
    keys[i] = y->keys[t-1]; // 提升中间关键字到当前节点
    n = n+1; // 当前节点关键字数量增加
}


int main() {
    BTree tree(322); 

    // 生成一些测试数据
    vector<int> keys;
    // 生成从 0 到 999 的序列
    for (int i = 0; i < 1001202; i++) {
        keys.push_back(i);
    }

    // 使用现代 C++ 随机引擎打乱
    random_device rd;
    mt19937 g(rd());
    shuffle(keys.begin(), keys.end(), g);

    // 保留前 100 个不重复的值
    keys.resize(1001);


    for (int key : keys) {
        tree.insert(key);
    }

    // 添加验证调用
    tree.validate();
    return 0;
}
