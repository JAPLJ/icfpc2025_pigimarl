use axum::{Router, routing::get};
use tokio::net::TcpListener;

use axum::{Json, extract::State, http::StatusCode, routing::post};
use serde::{Deserialize, Serialize};
use std::sync::{Arc, Mutex};

mod graph;
use graph::{DEGREE, Graph};

#[derive(Deserialize)]
struct SelectRequest {
    #[allow(dead_code)]
    id: String,
    #[serde(rename = "problemName")]
    problem_name: String,
}

#[derive(Serialize)]
struct SelectResponse {
    #[serde(rename = "problemName")]
    problem_name: String,
}

#[derive(Deserialize)]
struct ExploreRequest {
    #[allow(dead_code)]
    id: String,
    plans: Vec<String>,
}

#[derive(Serialize)]
struct ExploreResponse {
    results: Vec<Vec<usize>>,
    #[serde(rename = "queryCount")]
    query_count: usize,
}

#[derive(Deserialize)]
struct GuessRequest {
    #[allow(dead_code)]
    id: String,
    map: MapData,
}

#[derive(Deserialize)]
struct MapData {
    rooms: Vec<usize>,
    #[serde(rename = "startingRoom")]
    starting_room: usize,
    connections: Vec<Connection>,
}

#[derive(Deserialize)]
struct Connection {
    from: RoomDoor,
    to: RoomDoor,
}

#[derive(Deserialize)]
struct RoomDoor {
    room: usize,
    door: usize,
}

#[derive(Serialize)]
struct GuessResponse {
    correct: bool,
}

#[derive(Clone)]
struct AppState {
    graph: Graph,
    query_count: usize,
}

type SharedAppState = Arc<Mutex<AppState>>;

const PROBLEM_NAME_MAP: &[(&str, usize)] = &[
    ("probatio", 3),
    ("primus", 6),
    ("secundus", 12),
    ("tertius", 18),
    ("quartus", 24),
    ("quintus", 30),
];

fn get_n_from_problem_name(problem_name: &str) -> Option<usize> {
    PROBLEM_NAME_MAP
        .iter()
        .find(|(name, _)| *name == problem_name)
        .map(|(_, n)| *n)
}

// mapからGraphを作成
fn create_graph_from_map(map: &MapData) -> Option<Graph> {
    let n = map.rooms.len();
    let mut edges = vec![vec![!0; DEGREE]; n];

    for connection in &map.connections {
        let from_room = connection.from.room;
        let from_door = connection.from.door;
        let to_room = connection.to.room;
        let to_door = connection.to.door;

        if from_room >= n || to_room >= n || from_door >= DEGREE || to_door >= DEGREE {
            continue;
        }

        edges[from_room][from_door] = to_room;
        edges[to_room][to_door] = from_room;
    }

    for i in 0..n {
        for j in 0..DEGREE {
            if edges[i][j] == !0 {
                return None; // 辺不足
            }
        }
    }

    Some(Graph::new(map.rooms.clone(), edges, map.starting_room))
}

async fn select_handler(
    State(state): State<SharedAppState>,
    Json(payload): Json<SelectRequest>,
) -> Result<Json<SelectResponse>, (StatusCode, String)> {
    // problemNameからnを取得
    let n: usize = match get_n_from_problem_name(&payload.problem_name) {
        Some(val) => val,
        None => match payload.problem_name.parse() {
            Ok(val) => val,
            Err(_) => {
                return Err((
                    StatusCode::BAD_REQUEST,
                    format!("Invalid problemName: {}", payload.problem_name),
                ));
            }
        },
    };

    // ランダムなグラフを作成
    let mut app_state = state.lock().unwrap();
    app_state.query_count = 0;
    loop {
        app_state.graph = Graph::random(n);
        if app_state.graph.connected() {
            break;
        }
    }

    println!("n: {}", n);
    println!("graph: {:?}", app_state.graph);

    Ok(Json(SelectResponse {
        problem_name: payload.problem_name,
    }))
}

async fn explore_handler(
    State(state): State<SharedAppState>,
    Json(payload): Json<ExploreRequest>,
) -> Result<Json<ExploreResponse>, (StatusCode, String)> {
    let mut app_state = state.lock().unwrap();

    let n = app_state.graph.node_count();
    let max_length = n * 18;

    let mut results = Vec::new();

    let mut all_edge_ids: Vec<Vec<usize>> = Vec::new();

    for plan in &payload.plans {
        if plan.len() > max_length {
            return Err((StatusCode::BAD_REQUEST, format!("too long plan: {}", plan)));
        }
        let edge_ids: Result<Vec<usize>, _> = plan
            .chars()
            .map(|c| c.to_digit(10).map(|d| d as usize).ok_or(()))
            .collect();
        match edge_ids {
            Ok(ids) => all_edge_ids.push(ids),
            Err(_) => {
                return Err((StatusCode::BAD_REQUEST, format!("invalid plan: {}", plan)));
            }
        }
    }

    for edge_ids in &all_edge_ids {
        let walk_result = app_state.graph.walk(edge_ids.clone());
        results.push(walk_result);
    }

    app_state.query_count += payload.plans.len() + 1;

    Ok(Json(ExploreResponse {
        results,
        query_count: app_state.query_count,
    }))
}

async fn guess_handler(
    State(state): State<SharedAppState>,
    Json(payload): Json<GuessRequest>,
) -> Result<Json<GuessResponse>, (StatusCode, String)> {
    let app_state = state.lock().unwrap();

    // mapからGraphを作成
    let guess_graph = match create_graph_from_map(&payload.map) {
        Some(graph) => graph,
        None => {
            return Err((
                StatusCode::BAD_REQUEST,
                "invalid map: incomplete graph structure".to_string(),
            ));
        }
    };

    // bisimulationをチェック
    let correct = app_state.graph.bisimulation(&guess_graph);

    Ok(Json(GuessResponse { correct }))
}

// ルーターに/selectエンドポイントを追加し、Stateを注入
fn app_with_state(state: SharedAppState) -> Router {
    Router::new()
        .route("/select", post(select_handler))
        .route("/explore", post(explore_handler))
        .route("/guess", post(guess_handler))
        .with_state(state)
        .route("/", get(|| async { "Hello, World!" }))
}

// main関数を書き換え
#[tokio::main]
async fn main() {
    // 初期状態（頂点数1のグラフとquery_count=0で初期化）
    let state = Arc::new(Mutex::new(AppState {
        graph: Graph::random(1),
        query_count: 0,
    }));
    let app = app_with_state(state);
    axum::serve(TcpListener::bind("0.0.0.0:4567").await.unwrap(), app)
        .await
        .unwrap();
}
