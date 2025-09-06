// Random Walk + Identification
use std::collections::HashMap;

use crate::{api::Api, problem::DEGREE};
use anyhow::Result;
use rand::{Rng, SeedableRng};
use rand_pcg::Pcg64Mcg;

pub struct Solver {
    api: Api,
    n: usize,
}

impl Solver {
    pub fn new(problem_name: impl AsRef<str>, n: usize) -> Result<Self> {
        let api = Api::new();
        api.select(problem_name)?;
        Ok(Self { api, n })
    }

    pub fn solve(&self) -> Result<usize> {
        let mut rng = Pcg64Mcg::seed_from_u64(42);
        let random_walk_len = self.n * 5;
        let magic_len = self.n * 13 - 1;

        let mut random_walk = vec![];
        for _ in 0..random_walk_len {
            random_walk.push(rng.random_range(0..DEGREE));
        }

        let mut magic = vec![];
        for _ in 0..magic_len {
            magic.push(rng.random_range(0..DEGREE));
        }

        let mut first_plans = vec![];
        for walk in 0..=random_walk_len {
            let mut plan = vec![];
            plan.extend_from_slice(&random_walk[..walk]);
            plan.extend_from_slice(&magic);
            first_plans.push(plan);
        }

        let first_res = self.api.explore(&first_plans)?;
        let mut patterns = HashMap::new();
        let mut ixs = vec![];
        let mut labels = vec![];
        for (ix, res) in first_res.results.iter().enumerate() {
            let pattern = res[ix..]
                .iter()
                .map(|x| x.to_string())
                .collect::<Vec<String>>()
                .join("");
            if !patterns.contains_key(&pattern) {
                patterns.insert(pattern, ixs.len());
                ixs.push(ix);
                labels.push(res[ix]);
            }
        }

        if patterns.len() != self.n {
            return Err(anyhow::anyhow!("patterns.len() != self.n"));
        }

        let mut second_plans = vec![];
        for &ix in &ixs {
            for edge_id in 0..DEGREE {
                let mut plan = vec![];
                plan.extend_from_slice(&random_walk[..ix]);
                plan.push(edge_id);
                plan.extend_from_slice(&magic);
                second_plans.push(plan);
            }
        }

        let second_res = self.api.explore(&second_plans)?;
        let mut edges = vec![vec![!0; DEGREE]; self.n];
        for (i, &ix) in ixs.iter().enumerate() {
            for edge_id in 0..DEGREE {
                let pattern = second_res.results[DEGREE * i + edge_id][ix + 1..]
                    .iter()
                    .map(|x| x.to_string())
                    .collect::<Vec<String>>()
                    .join("");
                if let Some(&j) = patterns.get(&pattern) {
                    edges[i][edge_id] = j;
                } else {
                    return Err(anyhow::anyhow!("pattern not found"));
                }
            }
        }

        if !self.api.guess(&labels, &edges, 0)? {
            return Err(anyhow::anyhow!("guess failed"));
        }

        Ok(second_res.query_count)
    }
}
