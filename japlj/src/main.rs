use anyhow::Result;
use japlj::problem::ProblemName;
use japlj::solver4::Solver4;
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
        let solver = Solver4::new(&problem_name, n)?;
        println!("trial: {}", trial);
        match solver.solve() {
            Ok(query_count) => {
                println!("query_count: {}", query_count);
                break;
            }
            Err(e) => {
                println!("  failed: {:?}", e);
            }
        }
    }

    Ok(())
}
