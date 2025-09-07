use std::cmp;

// CSP (Backtracking)
use crate::{api::Api, problem::DEGREE};
use anyhow::Result;
use rand::{Rng, SeedableRng};
use rand_pcg::Pcg64Mcg;

#[derive(Debug, Clone)]
enum UndoOp {
    RemoveNewVertex(usize),
    RevertToId(usize),
    RemoveEdge(usize, usize),
    RevertAdj(usize, usize),
    RevertEdgeToLabel(usize, usize),
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct SearchState {
    labels: Vec<usize>,
    edges: Vec<Vec<usize>>,
    adj: Vec<Vec<bool>>,
    degree: Vec<usize>,
    to_id: Vec<usize>,
    edges_to_label: Vec<Vec<usize>>,
    assigned: usize,
}

impl SearchState {
    fn new(n: usize, rw_vertices: usize) -> Self {
        Self {
            labels: vec![!0; n],
            edges: vec![vec![!0; DEGREE]; n],
            adj: vec![vec![false; n]; n],
            degree: vec![0; n],
            to_id: vec![!0; rw_vertices],
            edges_to_label: vec![vec![!0; DEGREE]; n],
            assigned: 0,
        }
    }

    fn next_vertex(&self) -> usize {
        self.assigned
    }

    fn mergeable(&self, u: usize, walk: &[usize], walk_labels: &[usize], ix: usize) -> bool {
        assert!(u < self.assigned);
        if self.labels[u] != walk_labels[ix] {
            return false;
        }

        if ix > 0 && self.to_id[ix - 1] != !0 {
            let pu = self.to_id[ix - 1];
            let edge_id = walk[ix - 1];
            if self.edges[pu][edge_id] != !0 && self.edges[pu][edge_id] != u {
                return false;
            }
            if self.edges_to_label[pu][edge_id] != !0
                && self.edges_to_label[pu][edge_id] != walk_labels[ix]
            {
                return false;
            }
        }

        if ix < walk.len() {
            let edge_id = walk[ix];
            if self.edges_to_label[u][edge_id] != !0
                && self.edges_to_label[u][edge_id] != walk_labels[ix + 1]
            {
                return false;
            }
        }

        if ix < walk.len() && self.to_id[ix + 1] != !0 {
            let nu = self.to_id[ix + 1];
            let edge_id = walk[ix];
            if self.edges[u][edge_id] != !0 && self.edges[u][edge_id] != nu {
                return false;
            }
            if self.edges_to_label[u][edge_id] != !0
                && self.edges_to_label[u][edge_id] != walk_labels[ix + 1]
            {
                return false;
            }
        }

        true
    }

    fn merge(
        &mut self,
        u: usize,
        walk: &[usize],
        walk_labels: &[usize],
        ix: usize,
    ) -> (Vec<UndoOp>, bool) {
        let mut undo_ops = vec![];

        if u >= self.assigned {
            self.assigned += 1;
            self.labels[u] = walk_labels[ix];
            undo_ops.push(UndoOp::RemoveNewVertex(u));
        }

        self.to_id[ix] = u;
        undo_ops.push(UndoOp::RevertToId(ix));

        if ix > 0 && self.to_id[ix - 1] != !0 {
            let pu = self.to_id[ix - 1];
            let edge_id = walk[ix - 1];
            if self.edges[pu][edge_id] == !0 {
                self.edges[pu][edge_id] = u;
                undo_ops.push(UndoOp::RemoveEdge(pu, edge_id));
                if !self.adj[pu][u] {
                    self.adj[pu][u] = true;
                    self.adj[u][pu] = true;
                    self.degree[pu] += 1;
                    self.degree[u] += 1;
                    undo_ops.push(UndoOp::RevertAdj(pu, u));
                    if self.degree[pu] > DEGREE || self.degree[u] > DEGREE {
                        return (undo_ops, false);
                    }
                }
            }
        }

        if ix < walk.len() {
            let edge_id = walk[ix];
            if self.edges_to_label[u][edge_id] == !0 {
                self.edges_to_label[u][edge_id] = walk_labels[ix + 1];
                undo_ops.push(UndoOp::RevertEdgeToLabel(u, edge_id));
            }
        }

        if ix < walk.len() && self.to_id[ix + 1] != !0 {
            let nu = self.to_id[ix + 1];
            let edge_id = walk[ix];
            if self.edges[u][edge_id] == !0 {
                self.edges[u][edge_id] = nu;
                undo_ops.push(UndoOp::RemoveEdge(u, edge_id));
                if !self.adj[u][nu] {
                    self.adj[u][nu] = true;
                    self.adj[nu][u] = true;
                    self.degree[u] += 1;
                    self.degree[nu] += 1;
                    undo_ops.push(UndoOp::RevertAdj(u, nu));
                }
            }
        }

        (undo_ops, true)
    }

    fn undo(&mut self, undo_ops: Vec<UndoOp>) {
        for op in undo_ops.into_iter().rev() {
            match op {
                UndoOp::RemoveNewVertex(u) => {
                    self.assigned -= 1;
                    self.labels[u] = !0;
                }
                UndoOp::RevertToId(ix) => self.to_id[ix] = !0,
                UndoOp::RemoveEdge(pu, edge_id) => self.edges[pu][edge_id] = !0,
                UndoOp::RevertAdj(pu, u) => {
                    self.adj[pu][u] = false;
                    self.adj[u][pu] = false;
                    self.degree[pu] -= 1;
                    self.degree[u] -= 1;
                }
                UndoOp::RevertEdgeToLabel(u, edge_id) => {
                    self.edges_to_label[u][edge_id] = !0;
                }
            }
        }
    }
}

pub struct Solver5 {
    api: Api,
    n: usize,
    dfs_depth: usize,
    dfs_calls: usize,
    dfs_max_depth: usize,
}

impl Solver5 {
    pub fn new(problem_name: impl AsRef<str>, n: usize) -> Result<Self> {
        let api = Api::new();
        api.select(problem_name)?;
        Ok(Self {
            api,
            n,
            dfs_depth: 0,
            dfs_calls: 0,
            dfs_max_depth: 0,
        })
    }

    fn dfs(&mut self, state: &mut SearchState, walk: &[usize], walk_labels: &[usize]) -> bool {
        if self.dfs_calls > 200000 {
            return false;
        }

        self.dfs_calls += 1;
        let cur_depth = state.to_id.iter().filter(|&u| *u != !0).count();
        self.dfs_max_depth = self.dfs_max_depth.max(cur_depth);
        if self.dfs_calls % 100000 == 0 {
            println!("dfs_calls: {}", self.dfs_calls);
            println!("dfs_max_depth: {}", self.dfs_max_depth);
        }

        if state.to_id.iter().all(|&u| u != !0) {
            return true;
        }

        let mut ix = !0;
        let mut min_options = usize::MAX;
        for i in 0..walk_labels.len() {
            if state.to_id[i] != !0 {
                continue;
            }
            let mut num_options = 0;
            for u in 0..state.assigned {
                if state.mergeable(u, walk, walk_labels, i) {
                    num_options += 1;
                }
            }
            if state.assigned < self.n {
                num_options += 1;
            }
            if num_options < min_options {
                min_options = num_options;
                ix = i;
            }
        }

        let nu = state.next_vertex();
        let mut merge_candidates = (0..nu)
            .filter(|&u| state.labels[u] == walk_labels[ix])
            .collect::<Vec<_>>();
        merge_candidates.sort_unstable_by_key(|&u| cmp::Reverse(state.degree[u]));

        for u in merge_candidates {
            if state.mergeable(u, walk, walk_labels, ix) {
                let (undo_ops, success) = state.merge(u, walk, walk_labels, ix);
                if success && self.dfs(state, walk, walk_labels) {
                    return true;
                }
                state.undo(undo_ops);
            }
        }

        if nu < self.n {
            let (undo_ops, success) = state.merge(nu, walk, walk_labels, ix);
            if success && self.dfs(state, walk, walk_labels) {
                return true;
            }
            state.undo(undo_ops);
        }

        self.dfs_depth -= 1;
        false
    }

    fn reconstruct_graph(
        &mut self,
        walk: &[usize],
        walk_labels: &[usize],
    ) -> Result<(Vec<usize>, Vec<Vec<usize>>, Vec<Vec<bool>>)> {
        let mut state = SearchState::new(self.n, walk_labels.len());

        if !self.dfs(&mut state, walk, walk_labels) {
            return Err(anyhow::anyhow!("dfs failed"));
        }

        Ok((state.labels, state.edges, state.adj))
    }

    pub fn solve(&mut self) -> Result<usize> {
        let mut rng = Pcg64Mcg::seed_from_u64(42);
        let mut random_walk = vec![];
        for _ in 0..self.n * 18 {
            random_walk.push(rng.random_range(0..DEGREE));
        }

        let walk_labels = self.api.explore(&vec![random_walk.clone()])?.results[0].clone();
        let (labels, mut edges, adj) = self.reconstruct_graph(&random_walk, &walk_labels)?;

        println!("labels: {:?}", labels);
        println!("edges: {:?}", edges);
        println!("adj: {:?}", adj);

        for u in 0..self.n {
            let mut has = vec![false; self.n];
            for e in 0..DEGREE {
                if edges[u][e] != !0 {
                    has[edges[u][e]] = true;
                }
            }

            let mut no_edge = vec![];
            for v in 0..self.n {
                if adj[u][v] && !has[v] {
                    no_edge.push(v);
                }
            }

            for e in 0..DEGREE {
                if edges[u][e] == !0 {
                    edges[u][e] = no_edge.pop().unwrap_or(u);
                }
            }
        }

        if !self.api.guess(&labels, &edges, 0)? {
            return Err(anyhow::anyhow!("guess failed"));
        }

        Ok(2)
    }
}
