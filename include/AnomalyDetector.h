class AnomalyDetector{
    private:
        float _window[50];
        int _windowSize;
        int _windowIdx;
        float _windowAverage;
        int _sampleCount;
        float _rollingSum;
        float _varianceRollingSum;
        float _variance;
        float _standardDeviation;
        float _zScore;


    public:
        AnomalyDetector(int size){
            if (size > 50) {size = 50;}
            if (size == 0) {size = 1;}
            _windowSize = size;
            _windowIdx = 0;
            _sampleCount = 0;
            _zScore = 0.0f;

            for(int i = 0; i < 50; i++) {_window[i] = 0.0f;}
        }

    void update (float newTemp){
        _rollingSum = 0;
        _varianceRollingSum = 0;
        _window[_windowIdx] = newTemp;
        _windowIdx = (_windowIdx + 1) % _windowSize;
        _sampleCount ++;


        if (_sampleCount >=_windowSize){
            for (int i{0}; i <_windowSize; i++){
                _rollingSum += _window[i];
            }
            _windowAverage = _rollingSum / _windowSize;
            for (int i{0}; i<_windowSize; i++){
                _varianceRollingSum += (_window[i] - _windowAverage)*(_window[i] - _windowAverage);
            }
            _variance = _varianceRollingSum / _windowSize;
            _standardDeviation = sqrtf(_variance);

            //CALCULATE Z-SCORE - avoid dividing by 0 as that would crash ESP32
            if (_standardDeviation >0){
                _zScore = (newTemp - _windowAverage) / _standardDeviation;
            }   
            }
    }

    float getZscore() {return _zScore;}
    float getStdDev() {return _standardDeviation;}
    float getTemp() {
        if (_windowIdx == 0){return _window[_windowSize - 1];}
        else{return _window[_windowIdx - 1];}
        }
    bool isHotSpot() {
        if (_sampleCount < _windowSize){return false;}
        if (_zScore >= 3.0){return true;}
        else {return false;}
    }

};