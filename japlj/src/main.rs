use anyhow::Result;
use japlj::problem::ProblemName;
use japlj::solver::Solver;
use std::env;
use std::str::FromStr;

fn main() -> Result<()> {
    dotenvy::dotenv().ok();

    let args: Vec<String> = env::args().collect();
    let problem_name = if args.len() > 1 {
        args[1].clone()
    } else {
        return Err(anyhow::anyhow!("problem name is required"));
    };
    let n = ProblemName::from_str(&problem_name)
        .map(|pn| pn.nodes())
        .unwrap_or_else(|_| problem_name.parse::<usize>().unwrap());

    for trial in 0..100 {
        let solver = Solver::new(&problem_name, n)?;
        println!("trial: {}", trial);
        if let Ok(query_count) = solver.solve() {
            println!("query_count: {}", query_count);
            break;
        }
    }

    Ok(())
}
