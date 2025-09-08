use std::cmp;

// CSP (Backtracking)
use crate::{
    api::Api,
    problem::{DEGREE, LABELS},
};
use anyhow::Result;
use rand::{Rng, SeedableRng};
use rand_pcg::Pcg64Mcg;

#[derive(Debug, Clone)]
enum UndoOp {
    RemoveNewVertex(usize),
    RevertToId(usize),
    RemoveEdge(usize, usize),
    RevertEdgeToLabel(usize, usize),
    UnsetRevEdge(usize, usize, usize, usize),
    UnsetLonelyEdge(usize, usize),
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct SearchState {
    labels: Vec<usize>,
    label_counts: Vec<usize>,
    edges: Vec<Vec<(usize, usize)>>,
    adj: Vec<Vec<usize>>,
    degree: Vec<usize>,
    to_id: Vec<usize>,
    from_id: Vec<Vec<usize>>,
    matches: Vec<Vec<bool>>,
    edges_to_label: Vec<Vec<usize>>,
    assigned: usize,
}

impl SearchState {
    fn new(n: usize, walk: &[usize], walk_labels: &[usize]) -> Self {
        let rw_vertices = walk_labels.len();
        let mut matches = vec![vec![false; rw_vertices]; rw_vertices];
        for i in 0..rw_vertices {
            for j in 0..rw_vertices {
                matches[i][j] = true;
                for d in 0.. {
                    matches[i][j] &= walk_labels[i + d] == walk_labels[j + d];
                    if i.max(j) + d >= walk.len() || walk[i + d] != walk[j + d] {
                        break;
                    }
                }
            }
        }
        Self {
            labels: vec![!0; n],
            label_counts: vec![0; LABELS],
            edges: vec![vec![(!0, !0); DEGREE]; n],
            adj: vec![vec![0; n]; n],
            degree: vec![0; n],
            to_id: vec![!0; rw_vertices],
            from_id: vec![vec![]; n],
            matches,
            edges_to_label: vec![vec![!0; DEGREE]; n],
            assigned: 0,
        }
    }

    fn next_vertex(&self) -> usize {
        self.assigned
    }

    fn merge_score(&self, u: usize, walk: &[usize], walk_labels: &[usize], ix: usize) -> i32 {
        let mut score = 0;
        if ix > 0 && self.to_id[ix - 1] != !0 {
            let pu = self.to_id[ix - 1];
            let edge_id = walk[ix - 1];
            if self.edges[pu][edge_id].0 == u {
                score += 100;
            }
            if self.adj[u][pu] > 0 {
                score += 10;
            }
        }
        if ix < walk.len() {
            let edge_id = walk[ix];
            if self.edges_to_label[u][edge_id] == walk_labels[ix + 1] {
                score += 1;
            }
        }
        if ix < walk.len() && self.to_id[ix + 1] != !0 {
            let nu = self.to_id[ix + 1];
            let edge_id = walk[ix];
            if self.edges[u][edge_id].0 == nu {
                score += 100;
            }
            if self.adj[nu][u] > 0 {
                score += 10;
            }
        }
        score
    }

    fn mergeable(&self, u: usize, walk: &[usize], walk_labels: &[usize], ix: usize) -> bool {
        if u < self.assigned {
            if self.labels[u] != walk_labels[ix] {
                return false;
            }
            for &jx in self.from_id[u].iter() {
                if !self.matches[jx][ix] {
                    return false;
                }
            }
        }

        if ix > 0 && self.to_id[ix - 1] != !0 {
            let pu = self.to_id[ix - 1];
            let edge_id = walk[ix - 1];
            if self.edges[pu][edge_id].0 != !0 && self.edges[pu][edge_id].0 != u {
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
            if self.edges[u][edge_id].0 != !0 && self.edges[u][edge_id].0 != nu {
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
            self.label_counts[walk_labels[ix]] += 1;
            undo_ops.push(UndoOp::RemoveNewVertex(u));

            let labels_max = self.labels.len() / LABELS
                + if walk_labels[ix] < self.labels.len() % LABELS {
                    1
                } else {
                    0
                };
            if self.label_counts[walk_labels[ix]] > labels_max {
                return (undo_ops, false);
            }
        }

        self.to_id[ix] = u;
        self.from_id[u].push(ix);
        undo_ops.push(UndoOp::RevertToId(ix));

        if ix > 0 && self.to_id[ix - 1] != !0 {
            let pu = self.to_id[ix - 1];
            let edge_id = walk[ix - 1];
            if self.edges[pu][edge_id].0 == !0 {
                self.edges[pu][edge_id].0 = u;
                undo_ops.push(UndoOp::RemoveEdge(pu, edge_id));
                if self.adj[u][pu] > 0 {
                    self.adj[u][pu] -= 1;
                    let rev_e = self.edges[u].iter().position(|&e| e == (pu, !0)).unwrap();
                    self.edges[pu][edge_id].1 = rev_e;
                    self.edges[u][rev_e].1 = edge_id;
                    undo_ops.push(UndoOp::UnsetRevEdge(pu, edge_id, u, rev_e));
                } else {
                    if self.degree[pu] >= DEGREE || self.degree[u] >= DEGREE {
                        return (undo_ops, false);
                    }
                    self.adj[pu][u] += 1;
                    self.degree[pu] += 1;
                    self.degree[u] += 1;
                    undo_ops.push(UndoOp::UnsetLonelyEdge(pu, u));
                }
            } else if self.edges[pu][edge_id].0 != u {
                return (undo_ops, false);
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
            if self.edges[u][edge_id].0 == !0 {
                self.edges[u][edge_id].0 = nu;
                undo_ops.push(UndoOp::RemoveEdge(u, edge_id));
                if self.adj[nu][u] > 0 {
                    self.adj[nu][u] -= 1;
                    let rev_e = self.edges[nu].iter().position(|&e| e == (u, !0)).unwrap();
                    self.edges[u][edge_id].1 = rev_e;
                    self.edges[nu][rev_e].1 = edge_id;
                    undo_ops.push(UndoOp::UnsetRevEdge(u, edge_id, nu, rev_e));
                } else {
                    if self.degree[u] >= DEGREE || self.degree[nu] >= DEGREE {
                        return (undo_ops, false);
                    }
                    self.adj[u][nu] += 1;
                    self.degree[u] += 1;
                    self.degree[nu] += 1;
                    undo_ops.push(UndoOp::UnsetLonelyEdge(u, nu));
                }
            } else if self.edges[u][edge_id].0 != nu {
                return (undo_ops, false);
            }
        }

        (undo_ops, true)
    }

    fn undo(&mut self, undo_ops: Vec<UndoOp>) {
        for op in undo_ops.into_iter().rev() {
            match op {
                UndoOp::RemoveNewVertex(u) => {
                    self.assigned -= 1;
                    self.label_counts[self.labels[u]] -= 1;
                    self.labels[u] = !0;
                }
                UndoOp::RevertToId(ix) => {
                    self.from_id[self.to_id[ix]].pop();
                    self.to_id[ix] = !0;
                }
                UndoOp::RemoveEdge(pu, edge_id) => self.edges[pu][edge_id] = (!0, !0),
                UndoOp::RevertEdgeToLabel(u, edge_id) => {
                    self.edges_to_label[u][edge_id] = !0;
                }
                UndoOp::UnsetRevEdge(pu, edge_id, u, rev_e) => {
                    self.edges[pu][edge_id].1 = !0;
                    self.edges[u][rev_e].1 = !0;
                    self.adj[u][pu] += 1;
                }
                UndoOp::UnsetLonelyEdge(pu, u) => {
                    self.adj[pu][u] -= 1;
                    self.degree[pu] -= 1;
                    self.degree[u] -= 1;
                }
            }
        }
    }
}

pub struct Solver6 {
    api: Api,
    n: usize,
    dfs_depth: usize,
    dfs_calls: usize,
    dfs_max_depth: usize,
    rng: Pcg64Mcg,
}

impl Solver6 {
    pub fn new(problem_name: impl AsRef<str>, n: usize) -> Result<Self> {
        let api = Api::new();
        api.select(problem_name)?;
        Ok(Self {
            api,
            n,
            dfs_depth: 0,
            dfs_calls: 0,
            dfs_max_depth: 0,
            rng: Pcg64Mcg::seed_from_u64(42),
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
        merge_candidates.sort_unstable_by_key(|&u| {
            (
                cmp::Reverse(state.merge_score(u, walk, walk_labels, ix)),
                cmp::Reverse(state.degree[u]),
                self.rng.random_range(0..1000),
            )
        });

        for u in merge_candidates {
            if state.mergeable(u, walk, walk_labels, ix) {
                let (undo_ops, success) = state.merge(u, walk, walk_labels, ix);
                if success && self.dfs(state, walk, walk_labels) {
                    return true;
                }
                state.undo(undo_ops);
            }
        }

        if nu < self.n && state.mergeable(nu, walk, walk_labels, ix) {
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
    ) -> Result<(Vec<usize>, Vec<Vec<usize>>, Vec<Vec<usize>>)> {
        let mut state = SearchState::new(self.n, walk, walk_labels);

        if !self.dfs(&mut state, walk, walk_labels) {
            return Err(anyhow::anyhow!("dfs failed"));
        }

        let mut edges = vec![vec![!0; DEGREE]; self.n];
        for u in 0..self.n {
            for e in 0..DEGREE {
                edges[u][e] = state.edges[u][e].0;
            }
        }
        Ok((state.labels, edges, state.adj))
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
            let mut no_edge = vec![];
            for v in 0..self.n {
                for _ in 0..adj[v][u] {
                    no_edge.push(v);
                }
            }

            for e in 0..DEGREE {
                if edges[u][e] == !0 {
                    edges[u][e] = no_edge.pop().unwrap_or(u);
                }
            }
        }

        if labels.iter().any(|&l| l == !0) {
            return Err(anyhow::anyhow!("labels contain !0"));
        }

        if !self.api.guess(&labels, &edges, 0)? {
            return Err(anyhow::anyhow!("guess failed"));
        }

        Ok(2)
    }
}
