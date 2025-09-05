use std::collections::{HashMap, HashSet, VecDeque};

use rand::{Rng, SeedableRng, seq::SliceRandom};
use rand_pcg::Pcg64Mcg;

pub const LABELS: usize = 4;
pub const DEGREE: usize = 6;

#[derive(Debug, Clone)]
pub struct Graph {
    labels: Vec<usize>,
    edges: Vec<Vec<usize>>,
    start_node: usize,
}

impl Graph {
    pub fn new(labels: Vec<usize>, edges: Vec<Vec<usize>>, start_node: usize) -> Self {
        Self {
            labels,
            edges,
            start_node,
        }
    }

    pub fn random(n: usize) -> Self {
        let mut rng = Pcg64Mcg::from_os_rng();

        // labelsを初期化（0,1,2,3を均等に割り当ててシャッフル）
        let mut labels: Vec<usize> = (0..n).map(|i| i % LABELS).collect();
        labels.shuffle(&mut rng);

        // edgesを初期化（各ノードから他のノードにランダムに接続）
        let mut edges: Vec<Vec<usize>> = vec![Vec::new(); n];
        for i in 0..n {
            while edges[i].len() < DEGREE {
                let j = rng.random_range(0..n);
                let ok =
                    (i != j && edges[j].len() < DEGREE) || (i == j && edges[j].len() < DEGREE - 1);
                if ok {
                    edges[i].push(j);
                    edges[j].push(i);
                }
            }
            edges[i].shuffle(&mut rng);
        }

        Self::new(labels, edges, rng.random_range(0..n))
    }

    pub fn connected(&self) -> bool {
        let mut visited = vec![false; self.labels.len()];
        let mut queue = VecDeque::new();
        queue.push_back(self.start_node);
        visited[self.start_node] = true;
        while let Some(node) = queue.pop_front() {
            for edge_id in 0..DEGREE {
                let next_node = self.edges[node][edge_id];
                if !visited[next_node] {
                    visited[next_node] = true;
                    queue.push_back(next_node);
                }
            }
        }
        visited.iter().all(|&v| v)
    }

    pub fn node_count(&self) -> usize {
        self.labels.len()
    }

    pub fn walk(&self, edge_ids: Vec<usize>) -> Vec<usize> {
        let mut result = Vec::new();
        let mut current_node = self.start_node;
        result.push(self.labels[current_node]);
        for edge_id in edge_ids {
            let next_node = self.edges[current_node][edge_id];
            result.push(self.labels[next_node]);
            current_node = next_node;
        }
        result
    }

    pub fn bisimulation(&self, other: &Graph) -> bool {
        let n = self.labels.len();
        if n != other.labels.len() {
            return false;
        }

        let mut group_id = vec![0; n * 2];
        let mut groups = vec![HashSet::new(); LABELS];
        for i in 0..n {
            group_id[i] = self.labels[i];
            group_id[i + n] = other.labels[i];
            groups[group_id[i]].insert(i);
            groups[group_id[i + n]].insert(i + n);
        }

        let mut fresh_group_id = LABELS;
        let mut partitioner = VecDeque::new();
        for g in &groups {
            partitioner.push_back(g.clone());
        }

        while let Some(g) = partitioner.pop_front() {
            for edge_id in 0..DEGREE {
                let mut gep = HashMap::new();
                for i in 0..n {
                    if g.contains(&self.edges[i][edge_id]) {
                        gep.entry(group_id[i]).or_insert(HashSet::new()).insert(i);
                    }
                    if g.contains(&(other.edges[i][edge_id] + n)) {
                        gep.entry(group_id[i + n])
                            .or_insert(HashSet::new())
                            .insert(i + n);
                    }
                }

                for (gid, enter_g) in gep {
                    if groups[gid].len() != enter_g.len() {
                        for &v in &enter_g {
                            group_id[v] = fresh_group_id;
                            groups[gid].remove(&v);
                        }
                        groups.push(enter_g.clone());
                        fresh_group_id += 1;

                        partitioner.push_back(if enter_g.len() < groups[gid].len() {
                            enter_g
                        } else {
                            groups[gid].clone()
                        })
                    }
                }
            }
        }

        group_id[self.start_node] == group_id[other.start_node + n]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_bisimulation_example() {
        let graph1 = Graph::new(
            vec![0, 2, 1],
            vec![
                vec![2, 0, 2, 0, 0, 0],
                vec![1, 1, 1, 2, 2, 1],
                vec![2, 1, 1, 0, 2, 0],
            ],
            1,
        );

        let graph2 = Graph::new(
            vec![0, 1, 2],
            vec![
                vec![1, 0, 1, 0, 0, 0],
                vec![1, 2, 2, 0, 1, 0],
                vec![2, 2, 2, 1, 1, 2],
            ],
            2,
        );

        assert!(
            graph1.bisimulation(&graph2),
            "graph1 should be bisimilar to graph2"
        );
        assert!(
            graph2.bisimulation(&graph1),
            "graph2 should be bisimilar to graph1"
        );
    }
}
