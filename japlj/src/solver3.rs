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

        let discriminator_len = 6;
        let mut discriminator = vec![];
        for i in 0..discriminator_len {
            discriminator.push(WalkStep::Edge(i % DEGREE));
            discriminator.push(WalkStep::Rewrite(rng.random_range(0..LABELS)));
        }

        let random_walk_len = self.n * 6 - discriminator_len - 1;
        let (random_walk, first, ixs) = self.classify_vertices(&mut rng, random_walk_len)?;
        let mut rw_pos = vec![!0; self.n];
        let mut rw_last_pos = vec![!0; self.n];
        for i in 0..ixs.len() {
            rw_last_pos[ixs[i]] = i;
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

        let random_walk = random_walk
            .iter()
            .map(|x| WalkStep::Edge(*x))
            .collect::<Vec<WalkStep>>();

        let mut footprints = HashMap::new();
        let mut fp_plans = vec![];
        for u in 0..self.n {
            let mut plan = vec![];
            plan.extend_from_slice(&random_walk[..rw_pos[u]]);
            plan.extend_from_slice(&discriminator);
            fp_plans.push(plan);
        }
        let fp_res = self.api.explore_full(&fp_plans)?.results;
        let mut fps = vec![];
        for u in 0..self.n {
            let pattern = fp_res[u][rw_pos[u]..]
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<String>>()
                .join("");
            fps.push(pattern.clone());
            footprints.entry(pattern).or_insert(vec![]).push(u);
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
                plan.push(WalkStep::Edge(e));
                plan.extend_from_slice(&discriminator);
                edge_ids.push((u, e));
                edge_plans.push(plan);
            }
        }

        let edges_res = self.api.explore_full(&edge_plans)?.results;
        let mut edge_candidates = vec![];
        let mut edge_ok = true;
        for (&(u, e), res) in edge_ids.iter().zip(edges_res.iter()) {
            let pattern = res[rw_pos[u] + 1..]
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<String>>()
                .join("");
            if let Some(v) = footprints.get(&pattern) {
                if v.len() > 1 {
                    edge_candidates.push((u, e, v.clone()));
                    for &w in v {
                        edge_ok &= rw_pos[w] < rw_last_pos[u];
                    }
                } else {
                    edges[u][e] = v[0];
                }
            } else {
                return Err(anyhow::anyhow!("pattern not found"));
            }
        }
        if !edge_ok {
            return Err(anyhow::anyhow!("edge not ok"));
        }

        let mut edge_plans = vec![];
        let mut edge_plans_ixs = vec![];
        for (u, e, v) in edge_candidates {
            let mut base = 1;
            let mut candidate_ix = vec![!0; random_walk.len() + 1];
            for (i, &w) in v.iter().enumerate() {
                candidate_ix[rw_pos[w]] = i;
            }
            let start_ix = edge_plans.len();
            while base < v.len() {
                let mut plan = vec![];
                for i in 0..rw_last_pos[u] {
                    if candidate_ix[i] != !0 {
                        plan.push(WalkStep::Rewrite(candidate_ix[i] / base % LABELS));
                    }
                    if i < random_walk.len() {
                        plan.push(random_walk[i]);
                    }
                }
                plan.push(WalkStep::Edge(e));
                edge_plans.push(plan);
                base *= LABELS;
            }
            edge_plans_ixs.push((u, e, v.clone(), start_ix, edge_plans.len()));
        }

        let final_res = self.api.explore_full(&edge_plans)?;
        let (query_count, edges_res) = (final_res.query_count, final_res.results);
        for (u, e, v, start_ix, end_ix) in edge_plans_ixs {
            let mut cand_ix = 0;
            for i in (start_ix..end_ix).rev() {
                let label = edges_res[i][edges_res[i].len() - 1];
                cand_ix = cand_ix * LABELS + label;
            }
            println!("cands[{}][{}] = {:?} => {}", u, e, v, cand_ix);
            edges[u][e] = v[cand_ix];
        }

        for u in 0..self.n {
            println!("edges[{}]: {:?}", u, edges[u]);
        }

        if !self.api.guess(&labels, &edges, 0)? {
            return Err(anyhow::anyhow!("guess failed"));
        }

        Ok(query_count)
    }
}
