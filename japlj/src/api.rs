use std::{env, fmt};

use anyhow::Result;
use reqwest::blocking::Client;
use serde::{Deserialize, Serialize};
use serde_json::json;

use crate::problem::DEGREE;

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum WalkStep {
    Edge(usize),
    Rewrite(usize),
}

impl fmt::Display for WalkStep {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            WalkStep::Edge(e) => write!(f, "{}", e),
            WalkStep::Rewrite(r) => write!(f, "[{}]", r),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ExploreResponse {
    pub results: Vec<Vec<usize>>,
    #[serde(rename = "queryCount")]
    pub query_count: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct GuessRequest {
    id: String,
    map: GuessGraph,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct GuessGraph {
    rooms: Vec<usize>,
    #[serde(rename = "startingRoom")]
    starting_room: usize,
    connections: Vec<GuessGraphConnection>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct GuessGraphConnection {
    from: RoomAndDoor,
    to: RoomAndDoor,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct RoomAndDoor {
    room: usize,
    door: usize,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct GuessResponse {
    correct: bool,
}

#[derive(Debug, Clone)]
pub struct Api {
    client: Client,
    base_url: String,
    credentials: String,
}

impl Api {
    pub fn new() -> Self {
        Self {
            client: Client::new(),
            base_url: env::var("BASE_URL").unwrap(),
            credentials: env::var("ID").unwrap(),
        }
    }

    pub fn select(&self, problem: impl AsRef<str>) -> Result<()> {
        self.client
            .post(format!("{}/select", self.base_url))
            .header("Content-Type", "application/json")
            .json(&json!({
                "id": self.credentials,
                "problemName": problem.as_ref(),
            }))
            .send()?;

        Ok(())
    }

    pub fn explore(&self, plans: &Vec<Vec<usize>>) -> Result<ExploreResponse> {
        let mut plans_str = vec![];
        for plan in plans {
            plans_str.push(
                plan.iter()
                    .map(|x| x.to_string())
                    .collect::<Vec<String>>()
                    .join(""),
            );
        }

        let response: ExploreResponse = self
            .client
            .post(format!("{}/explore", self.base_url))
            .header("Content-Type", "application/json")
            .json(&json!({
                "id": self.credentials,
                "plans": plans_str,
            }))
            .send()?
            .json()?;

        Ok(response)
    }

    pub fn explore_full(&self, plans: &Vec<Vec<WalkStep>>) -> Result<ExploreResponse> {
        let mut plans_str = vec![];
        for plan in plans {
            plans_str.push(
                plan.iter()
                    .map(|x| x.to_string())
                    .collect::<Vec<String>>()
                    .join(""),
            );
        }

        let response: ExploreResponse = self
            .client
            .post(format!("{}/explore", self.base_url))
            .header("Content-Type", "application/json")
            .json(&json!({
                "id": self.credentials,
                "plans": plans_str,
            }))
            .send()?
            .json()?;

        Ok(response)
    }

    pub fn guess(
        &self,
        labels: &Vec<usize>,
        edges: &Vec<Vec<usize>>,
        starting_room: usize,
    ) -> Result<bool> {
        let n = labels.len();
        let graph = {
            let mut connections = vec![];
            let mut paired = vec![vec![false; DEGREE]; n];
            for u in 0..n {
                for ui in 0..DEGREE {
                    if paired[u][ui] {
                        continue;
                    }

                    if edges[u][ui] == u {
                        paired[u][ui] = true;
                        connections.push(GuessGraphConnection {
                            from: RoomAndDoor { room: u, door: ui },
                            to: RoomAndDoor { room: u, door: ui },
                        });
                        continue;
                    }

                    let v = edges[u][ui];
                    let vi = {
                        let mut vi = None;
                        for i in 0..DEGREE {
                            if edges[v][i] == u && !paired[v][i] {
                                vi = Some(i);
                                break;
                            }
                        }
                        vi.ok_or(anyhow::anyhow!("vi not found: {} {}", u, v))?
                    };

                    paired[u][ui] = true;
                    paired[v][vi] = true;
                    connections.push(GuessGraphConnection {
                        from: RoomAndDoor { room: u, door: ui },
                        to: RoomAndDoor { room: v, door: vi },
                    });
                }
            }
            GuessGraph {
                rooms: labels.clone(),
                starting_room,
                connections,
            }
        };

        let request = GuessRequest {
            id: self.credentials.clone(),
            map: graph,
        };

        let response: GuessResponse = self
            .client
            .post(format!("{}/guess", self.base_url))
            .header("Content-Type", "application/json")
            .json(&request)
            .send()?
            .json()?;

        Ok(response.correct)
    }
}
