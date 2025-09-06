// CSP (z3)
use crate::{
    api::Api,
    problem::{DEGREE, LABELS},
};
use anyhow::Result;
use rand::{SeedableRng, seq::SliceRandom};
use rand_pcg::Pcg64Mcg;
use z3::{
    Config, FuncDecl, Sort,
    ast::{Bool, Int},
    with_z3_config,
};

pub struct Solver2 {
    api: Api,
    n: usize,
}

impl Solver2 {
    pub fn new(problem_name: impl AsRef<str>, n: usize) -> Result<Self> {
        let api = Api::new();
        api.select(problem_name)?;
        Ok(Self { api, n })
    }

    fn construct_graph(&self, plan: &Vec<usize>, res: &Vec<usize>) -> Result<bool> {
        let k = plan.len();

        let solver = z3::Solver::new();

        // Vertices
        let mut vs = vec![];
        for i in 0..=k {
            vs.push(Int::new_const(format!("v{}", i)));
            solver.assert(Bool::and(&[vs[i].ge(0), vs[i].lt(self.n as i64)]));
        }

        // Labels
        for i in 0..=k {
            for j in i + 1..=k {
                let mut possibly_same = res[i] == res[j];
                for d in 0..k - j {
                    if plan[i + d] != plan[j + d] {
                        break;
                    }
                    possibly_same &= res[i + d] == res[j + d];
                    if !possibly_same {
                        break;
                    }
                }
                if !possibly_same {
                    solver.assert(vs[i].eq(&vs[j]).not());
                }
            }
        }

        let label_of = FuncDecl::new("label_of", &[&Sort::int()], &Sort::int());
        for i in 0..=k {
            solver.assert(label_of.apply(&[&vs[i]]).eq(&Int::from_i64(res[i] as i64)));
        }

        for li in 0..LABELS {
            let target_count = self.n / LABELS + if li < self.n % LABELS { 1 } else { 0 };
            let mut count = vec![];
            for u in 0..self.n {
                count.push(
                    label_of
                        .apply(&[&Int::from_i64(u as i64)])
                        .eq(&Int::from_i64(li as i64))
                        .ite(&Int::from_i64(1), &Int::from_i64(0)),
                );
            }
            let count = Int::add(&count);
            solver.assert(count.eq(Int::from_i64(target_count as i64)));
        }

        // Edges
        for i in 0..k {
            for j in i + 1..k {
                if plan[i] == plan[j] && res[i] == res[j] {
                    solver.assert(vs[i].eq(&vs[j]).implies(vs[i + 1].eq(&vs[j + 1])));
                }
            }
        }

        // Regularities
        for u in 0..self.n {
            for e in 0..DEGREE {
                let mut cond = vec![];
                for i in 0..k {
                    if plan[i] == e {
                        cond.push(vs[i].eq(u as i64));
                    }
                }
                solver.assert(Bool::or(&cond));
            }
        }

        // Surjective
        for u in 0..self.n {
            let mut cond = vec![];
            for i in 0..=k {
                cond.push(vs[i].eq(u as i64));
            }
            solver.assert(Bool::or(&cond));
        }

        // Transition
        let tr = FuncDecl::new("tr", &[&Sort::int(), &Sort::int()], &Sort::int());
        for i in 0..k {
            solver.assert(
                tr.apply(&[&vs[i], &Int::from_i64(plan[i] as i64)])
                    .eq(&vs[i + 1]),
            );
        }

        for u in 0..self.n {
            for v in u + 1..self.n {
                let mut count_u = vec![];
                let mut count_v = vec![];
                for edge_id in 0..DEGREE {
                    count_u.push(
                        tr.apply(&[&Int::from_i64(u as i64), &Int::from_i64(edge_id as i64)])
                            .eq(&Int::from_i64(v as i64))
                            .ite(&Int::from_i64(1), &Int::from_i64(0)),
                    );
                    count_v.push(
                        tr.apply(&[&Int::from_i64(v as i64), &Int::from_i64(edge_id as i64)])
                            .eq(&Int::from_i64(u as i64))
                            .ite(&Int::from_i64(1), &Int::from_i64(0)),
                    );
                }
                let count_u = Int::add(&count_u);
                let count_v = Int::add(&count_v);
                solver.assert(count_u.eq(count_v));
            }
        }

        // Symmtery Breaking
        let mut sigs = vec![];
        for u in 0..self.n {
            let mut sig = vec![];
            sig.push(
                label_of
                    .apply(&[&Int::from_i64(u as i64)])
                    .as_int()
                    .unwrap(),
            );
            for e in 0..DEGREE {
                sig.push(
                    label_of
                        .apply(&[&tr.apply(&[&Int::from_i64(u as i64), &Int::from_i64(e as i64)])])
                        .as_int()
                        .unwrap(),
                );
            }
            sigs.push(sig);
        }
        for u in 0..self.n - 1 {
            let v = u + 1;
            let mut lex_le = vec![];
            for eqs in 0..=DEGREE {
                let mut cmps = vec![];
                for i in 0..eqs {
                    cmps.push(sigs[u][i].eq(&sigs[v][i]));
                }
                cmps.push(sigs[u][eqs].lt(&sigs[v][eqs]));
                lex_le.push(Bool::and(&cmps));
            }
            solver.assert(Bool::or(&lex_le));
        }

        let solution = solver
            .solutions(&vs, true)
            .next()
            .ok_or(anyhow::anyhow!("No solution found"))?;

        // Reconstruct
        let mut ixs = vec![];
        for i in 0..=k {
            let ix = solution[i]
                .as_i64()
                .ok_or(anyhow::anyhow!("Invalid solution"))?;
            ixs.push(ix as usize);
        }

        let mut labels = vec![!0; self.n];
        let mut edges = vec![vec![!0; DEGREE]; self.n];
        for i in 0..=k {
            labels[ixs[i]] = res[i];
        }
        for i in 0..k {
            let edge_id = plan[i];
            edges[ixs[i]][edge_id] = ixs[i + 1];
        }

        println!("res: {:?}", res);
        println!("ixs: {:?}", ixs);
        println!("labels: {:?}", labels);
        println!("edges: {:?}", edges);

        Ok(self.api.guess(&labels, &edges, ixs[0])?)
    }

    pub fn solve(&self) -> Result<usize> {
        let mut rng = Pcg64Mcg::seed_from_u64(42);

        let mut plan = vec![];
        for _ in 0..3 * self.n {
            for e in 0..DEGREE {
                plan.push(e);
            }
        }
        plan.shuffle(&mut rng);

        let res = self.api.explore(&vec![plan.clone()])?;

        let mut cfg = Config::new();
        cfg.set_timeout_msec(3000);

        if !with_z3_config(&cfg, || self.construct_graph(&plan, &res.results[0]))? {
            return Err(anyhow::anyhow!("construct_graph failed"));
        }

        Ok(2)
    }
}
