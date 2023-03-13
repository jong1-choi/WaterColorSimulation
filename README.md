# WaterColorSimulation

computer generated watercolor, Real-time simulation of watery paint, stable fluid 참고

처음에는 Kubelka Munk model만 구현해보려고 시작

아래는 Kubelka Munk model 결과들
<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/km1.png">
<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/km2.png">
<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/km3.png">

km model 사용해서 수채화 시뮬레이션 시작...

<img src="https://github.com/jong1-choi/WaterColorSimulation/blob/main/demo.gif">

boundary condition이 제대로 적용되지 않아 blooming 같은 효과들이 제대로 나타나지 않고 conservation이 되지 않아
찾아보던중 'Compact Poisson Filters for Fast Fluid Simulation' 논문에서 iteration 자체를 poisson filter로 해결하고
boundary condtion도 해결하는 방법을 제시하고 있어서 연구 후 적용시켜서 KM 모델까지 한 번에 구현 예정
