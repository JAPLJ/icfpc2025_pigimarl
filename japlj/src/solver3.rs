// [Full Division]
// Random Walk + Identification
use std::collections::HashMap;

use crate::{
    api::{Api, WalkStep},
    problem::{DEGREE, LABELS},
};
use anyhow::Result;
use rand::{Rng, SeedableRng};
use rand_pcg::Pcg64Mcg;

pub struct Solver3 {
    api: Api,
    n: usize,
}

impl Solver3 {
    pub fn new(problem_name: impl AsRef<str>, n: usize) -> Result<Self> {
        let api = Api::new();
        api.select(problem_name)?;
        Ok(Self { api, n })
    }

    fn classify_vertices(
        &self,
        rng: &mut Pcg64Mcg,
        random_walk_len: usize,
    ) -> Result<(Vec<usize>, Vec<usize>, Vec<usize>)> {
        let mut random_walk = vec![];
        for _ in 0..random_walk_len {
            random_walk.push(rng.random_range(0..DEGREE));
        }
        let first = self.api.explore(&vec![random_walk.clone()])?.results[0].clone();

        let mut walk = vec![];
        for i in 0..random_walk_len {
            walk.push(WalkStep::Rewrite((first[i] + 1) % LABELS));
            walk.push(WalkStep::Edge(random_walk[i]));
        }
        let second = self.api.explore_full(&vec![walk.clone()])?.results[0].clone();

        let mut repr = vec![];
        let mut ixs = vec![!0; random_walk_len + 1];
        let mut new_ix = 0;
        let mut pos_by_labels = vec![vec![]; LABELS];
        for i in 0..=random_walk_len {
            if first[i] == second[2 * i] {
                ixs[i] = new_ix;
                new_ix += 1;
                pos_by_labels[first[i]].push(i);
                repr.push(true);
            } else {
                repr.push(false);
            }
        }

        let mut walks = vec![];
        let mut label_pos = vec![];
        let mut base = 1;
        while base < self.n.div_ceil(LABELS) {
            let mut counter = vec![0; LABELS];
            let mut walk = vec![];
            let mut pos = vec![];
            for i in 0..=random_walk_len {
                pos.push(walk.len());
                if repr[i] {
                    let li = first[i];
                    walk.push(WalkStep::Rewrite(counter[li] / base % LABELS));
                    counter[li] += 1;
                }
                if i < random_walk_len {
                    walk.push(WalkStep::Edge(random_walk[i]));
                }
            }
            walks.push(walk);
            label_pos.push(pos);
            base *= LABELS;
        }

        let third = self.api.explore_full(&walks)?;
        for i in 0..=random_walk_len {
            if repr[i] {
                continue;
            }
            let mut repr_num = 0;
            for j in (0..walks.len()).rev() {
                let label = third.results[j][label_pos[j][i]];
                repr_num = repr_num * LABELS + label;
            }
            ixs[i] = ixs[pos_by_labels[first[i]][repr_num]];
        }

        Ok((random_walk, first, ixs))
    }

    pub fn solve(&self) -> Result<usize> {
        let mut rng = Pcg64Mcg::seed_from_u64(42);

        let mut labels = vec![!0; self.n];
        let mut edges = vec![vec![!0; DEGREE]; self.n];

        let discriminator_len = 10;
        let mut discriminator = vec![];
        for i in 0..discriminator_len {
            discriminator.push(i % DEGREE);
            // discriminator.push(rng.random_range(0..DEGREE));
        }

        let random_walk_len = self.n * 6 - discriminator_len - 1;
        let (random_walk, first, ixs) = self.classify_vertices(&mut rng, random_walk_len)?;
        let mut rw_pos = vec![!0; self.n];
        for i in 0..ixs.len() {
            if rw_pos[ixs[i]] == !0 {
                rw_pos[ixs[i]] = i;
                labels[ixs[i]] = first[i];
            }
            if i < random_walk.len() {
                edges[ixs[i]][random_walk[i]] = ixs[i + 1];
            }
        }
        if rw_pos.iter().any(|x| *x == !0) {
            return Err(anyhow::anyhow!("cannot distinguish vertices"));
        }

        let mut footprints = HashMap::new();
        let mut fp_plans = vec![];
        for u in 0..self.n {
            let mut plan = vec![];
            plan.extend_from_slice(&random_walk[..rw_pos[u]]);
            plan.extend_from_slice(&discriminator);
            fp_plans.push(plan);
        }
        let fp_res = self.api.explore(&fp_plans)?.results;
        for u in 0..self.n {
            let pattern = fp_res[u][rw_pos[u]..]
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<String>>()
                .join("");
            if !footprints.contains_key(&pattern) {
                footprints.insert(pattern, u);
            } else {
                return Err(anyhow::anyhow!("footprint collision"));
            }
        }

        let mut edge_ids = vec![];
        let mut edge_plans = vec![];
        for u in 0..self.n {
            for e in 0..DEGREE {
                if edges[u][e] != !0 {
                    continue;
                }
                let mut plan = vec![];
                plan.extend_from_slice(&random_walk[..rw_pos[u]]);
                plan.push(e);
                plan.extend_from_slice(&discriminator);
                edge_ids.push((u, e));
                edge_plans.push(plan);
            }
        }

        let final_res = self.api.explore(&edge_plans)?;
        let (query_count, edges_res) = (final_res.query_count, final_res.results);

        for (&(u, e), res) in edge_ids.iter().zip(edges_res.iter()) {
            let pattern = res[rw_pos[u] + 1..]
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<String>>()
                .join("");
            if let Some(&v) = footprints.get(&pattern) {
                edges[u][e] = v;
            } else {
                return Err(anyhow::anyhow!("pattern not found"));
            }
        }

        if !self.api.guess(&labels, &edges, 0)? {
            return Err(anyhow::anyhow!("guess failed"));
        }

        Ok(query_count)
    }
}
