import requests
from schema import (
    ExploreRequest,
    ExploreResponse,
    GuessRequest,
    GuessResponse,
    SelectRequest,
    SelectResponse,
)


class ApiHandler:
    def __init__(self, api_domain: str, request_timeout: int):
        self.api_domain = api_domain
        self.request_timeout = request_timeout

    def select(self, select_request: SelectRequest) -> SelectResponse:
        url = f"{self.api_domain}/select"
        response = requests.post(
            url,
            headers={"Content-Type": "application/json"},
            json=select_request.model_dump(by_alias=True),
            timeout=self.request_timeout,
        )
        if response.status_code != 200:
            raise Exception(f"Failed to invoke /select endpoint: {response.text}")
        return SelectResponse.model_validate(response.json(), by_alias=True)

    def explore(self, explore_request: ExploreRequest) -> ExploreResponse:
        url = f"{self.api_domain}/explore"
        response = requests.post(
            url,
            headers={"Content-Type": "application/json"},
            json=explore_request.model_dump(by_alias=True),
            timeout=self.request_timeout,
        )
        if response.status_code != 200:
            raise Exception(f"Failed to invoke /explore endpoint: {response.text}")
        return ExploreResponse.model_validate(response.json(), by_alias=True)

    def guess(self, guess_request: GuessRequest) -> GuessResponse:
        url = f"{self.api_domain}/guess"
        response = requests.post(
            url,
            headers={"Content-Type": "application/json"},
            json=guess_request.model_dump(by_alias=True),
            timeout=self.request_timeout,
        )
        if response.status_code != 200:
            raise Exception(f"Failed to invoke /guess endpoint: {response.text}")
        return GuessResponse.model_validate(response.json(), by_alias=True)
