#include <bits/stdc++.h>
using namespace std;

// 输入格式（0-base）：
// n m
// 接下来 m 行，每行 u v 表示有向边 u -> v（顶点编号 0..n-1）
// 如果你的输入是 1-base ，请在输入时将顶点编号减 1。

int main() {
	ios::sync_with_stdio(false);
	cin.tie(nullptr);

	int n, m;
	if (!(cin >> n >> m)) {
		cerr << "读取 n m 失败，请按格式输入顶点数和 list 长度 (或 n m 后跟边列表)。\n";
		return 1;
	}

	// 读取剩余所有整数到缓冲区，以便我们判断输入是紧缩邻接表还是边列表
	vector<int> rest;
	int x;
	while (cin >> x) rest.push_back(x);

	vector<int> h;
	vector<int> list;

	bool parsed_compressed = false;

	// 尝试识别紧缩邻接表格式：接下来的 n+1 个数为 h[]，且 h[n] 等于剩余元素数（list 的大小）
	if ((int)rest.size() >= n + 1) {
		int possible_list_size = rest[n];
		if (possible_list_size >= 0 && possible_list_size == (int)rest.size() - (n + 1)) {
			// 解释为紧缩邻接表
			h.assign(rest.begin(), rest.begin() + (n + 1));
			list.assign(rest.begin() + (n + 1), rest.end());
			parsed_compressed = true;
			cerr << "Detected compressed adjacency input (h + list).\n";
		}
	}

	if (!parsed_compressed) {
		// 回退到边列表格式：rest 应包含 2*m 个整数作为 m 对 (u,v)
		if ((int)rest.size() == 2 * m) {
			vector<vector<int>> adj(n);
			for (int i = 0; i < m; ++i) {
				int u = rest[2 * i];
				int v = rest[2 * i + 1];
				if (u < 0 || u >= n || v < 0 || v >= n) {
					cerr << "边端点超出范围: " << u << " -> " << v << "\n";
					return 1;
				}
				adj[u].push_back(v);
			}
			h.assign(n + 1, 0);
			for (int i = 0; i < n; ++i) h[i + 1] = h[i] + (int)adj[i].size();
			list.reserve(m);
			for (int i = 0; i < n; ++i) for (int vtx : adj[i]) list.push_back(vtx);
			cerr << "Detected edge-list input (u v pairs). Converted to compressed form.\n";
		} else {
			cerr << "输入格式无法识别。期望紧缩邻接表 (n m then h[0..n] and list[m]) 或边列表 (n m then m pairs)。\n";
			return 1;
		}
	}

	// DFS 拓扑排序（递归），使用 stack 存储拓扑序（逆后序）
	vector<int> state(n, 0); // 0 = 未访问, 1 = 在栈中(访问中), 2 = 完成
	stack<int> order;
	function<bool(int)> dfs = [&](int u) -> bool {
		state[u] = 1;
		for (int idx = h[u]; idx < h[u+1]; ++idx) {
			int v = list[idx];
			if (state[v] == 1) return false; // 找到环
			if (state[v] == 0) {
				if (!dfs(v)) return false;
			}
		}
		state[u] = 2;
		order.push(u);
		return true;
	};

	for (int i = 0; i < n; ++i) {
		if (state[i] == 0) {
			if (!dfs(i)) {
				cout << "Graph has a cycle; topological order does not exist.\n";
				return 0;
			}
		}
	}

	// 输出紧缩邻接表（可选，可帮助验证）
	// 输出格式：h（n+1 个），list（m 个）
	cout << "h:";
	for (int x : h) cout << ' ' << x;
	cout << '\n';
	cout << "list:";
	for (int x : list) cout << ' ' << x;
	cout << '\n';

	// 输出拓扑序（从前到后）
	vector<int> topo;
	topo.reserve(n);
	while (!order.empty()) { topo.push_back(order.top()); order.pop(); }

	cout << "Topological order:";
	for (int v : topo) cout << ' ' << v;
	cout << '\n';

	return 0;
}

