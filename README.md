# WaterColorSimulation

computer generated watercolor, Real-time simulation of watery paint, stable fluid 참고

처음에는 Kubelka Munk model만 구현해보려고 시작

아래는 Kubelka Munk model 결과들
<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/Images/km1.png">
<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/Images/km2.png">
<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/Images/km3.png">

km model 사용해서 수채화 시뮬레이션 시작...

<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/Images/demo.gif" width="500" height="400">
먼저 종이 위의 물과 염료의 이동은 3개의 층을 통해 진행되는데 물과 염료가 이동하는 fluid layer, 염료들이 종이에 부착되고 탈착되는 과정을 반복하는 surface layer, 마지막으로 종이가 물을 흡수하고 퍼져나가는 과정을 나타내는 capillary layer로 구성됩니다.

![스크린샷 2024-06-08 오후 9.53.33.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2024-06-08_%25EC%2598%25A4%25ED%259B%2584_9.53.33.png)

종이는 최대 최소가 정해진 periln noise를 사용해 샘플링되었으며 여기서 종이의 높이는 이 후 염료의 흡착률 및 탈착률, 종이의 유제 저장 정도, diffuse로 인한 속도에 영향을 주게됩니다.

먼저 fluid layer에서는 스트로크에 의해 물, 염료, 속도값이 추가되며(현재 구현체는 스트로크에 의한 속도 미구현) 이후 속도 필드를 업데이트하고 물의 양을 업데이트해 마지막으로 염료의 양을 업데이트합니다.

속도 필드를 업데이트 하는 과정은 implicit euler method를 사용한 navier strokes 방적식을 기반으로 diffuse term과 advection term을 계산하고, 후에 물의 양을 업데이트 할 때 diffuse를 따로 계산하지 않고 물의 높이에 따른 차이를 속도 필드에 추가하여 속도 필드를 업데이트합니다.

![스크린샷 2023-01-16 오후 5.44.53.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-16_%25EC%2598%25A4%25ED%259B%2584_5.44.53.png)

![스크린샷 2023-01-17 오후 3.07.29.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-17_%25EC%2598%25A4%25ED%259B%2584_3.07.29.png)

![스크린샷 2023-01-18 오후 9.33.24.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-18_%25EC%2598%25A4%25ED%259B%2584_9.33.24.png)

물의 양을 업데이트하는 과정은 앞서 구한 속도 필드에 대해 주변 셀의 각각 평균 속도를 구하고 물의 높이와 셀의 크기를 사용해 타임 스텝마다 이동하는 물의 양을 계산합니다. 이 과정을 주변 4개의 셀에 대해 수행하고 모든 셀은 물의 변화에 1/4씩 기여한다고 가정해 교환량을 계산해줍니다.

![스크린샷 2023-01-18 오후 3.40.47.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-18_%25EC%2598%25A4%25ED%259B%2584_3.40.47.png)

![스크린샷 2023-01-18 오후 3.41.34.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-18_%25EC%2598%25A4%25ED%259B%2584_3.41.34.png)

물의 증발은 물 표면의 가장자리에 가까워 질 수록 증가한다는 사실을 이용하여 k x k의 크기를 가진 가우시안 필터를 각 셀에 적용해 증발률을 계산하였습니다.

![스크린샷 2022-11-28 오후 3.11.17.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2022-11-28_%25EC%2598%25A4%25ED%259B%2584_3.11.17.png)

마지막으로 염료의 양은 물의 양을 업데이트 하는 방식과 동일하게 진행됩니다.

다음으로 suface layer에서는 해당 셀의 물의 양과 과립성, 밀도, 흡착률에 의해 각 염료들이 흡착되고 탈착되는 양을 계산하여 흡착되는 양만큼 해당 fluid layer의 염료들을 surface layer로 보내고 탈착되는 양을 계산하여 surface layer의 염료들을 fluid layer로 보내 염료의 움직임을 모델링합니다.

![스크린샷 2023-01-19 오전 12.09.35.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-19_%25EC%2598%25A4%25EC%25A0%2584_12.09.35.png)

![스크린샷 2023-01-19 오전 12.39.03.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2023-01-19_%25EC%2598%25A4%25EC%25A0%2584_12.39.03.png)

마지막으로 Capillary Layer에서는 종이에 물이 흡수되어 퍼져나가는 현상을 표현합니다.
이 방법에서 surface layer는 wet mask를 지정해 스트로크로 인해 물이 들어간 부분을 제외하고는 boundary condition을 두어 물이 이동하지 않게 합니다. 하지만 실제로는 종이를 타고 물이 퍼져가기 때문에 Capillary Layer에서 종이의 높이 따라 물의 수용량이 정해지게 되고, surface layer와 다르게 wet mask가 아닌 부분에도 diffuse가 발생하게 됩니다. 임계값을 정해 해당 셀의 저장공간이 특정값 이상이면 해당 셀을 wet mask로 처리해주고 저장 용량이 모두 찬 경우 확산을 받지 않게 하여 종이에 의한 물의 확산을 표현합니다.

![스크린샷 2022-11-28 오후 4.43.50.png](watercolor%20simulation%207dd0b9353d1e4b2cb9921764089f81a8/%25EC%258A%25A4%25ED%2581%25AC%25EB%25A6%25B0%25EC%2583%25B7_2022-11-28_%25EC%2598%25A4%25ED%259B%2584_4.43.50.png)
