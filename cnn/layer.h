/*
* layer.h
*/
#include <iostream>
#include <map>
#include <vector>

namespace sub_dl {

enum {
    CONV = 0,
    POOL,
    FULLCONN,
	INPUT,
	LOSS,
};

class Layer {

private:
    std::vector<matrix_double> _data;
    std::vector<matrix_double> _errors;
    std::vector<matrix_double> _weights;
    int _type;

public:
    
    int _feature_x_dim;
    int _feature_y_dim;

    virtual void _forward(Layer* _pre_layer) = 0;
    virtual void _backward(Layer* _nxt_layer) = 0;

}


class ConvLayer : public Layer {

private:
    int _input_dim;
    int _output_dim;
    int _kernel_x_dim;
    int _kernel_y_dim;
    matrix_int _conn_map;
    matrix_double _bias;
    Matrix<matrix_double> _conv_kernels;

public:

    virtual ~ConvLayer() {}
    ConvLayer() {}

    ConvLayer(int input_dim, int output_dim, int kernel_x_dim, int kernel_y_dim) : 
        _input_dim(input_dim), _output_dim(output_dim),
        _kernel_x_dim(kernel_x_dim), _kernel_y_dim(kernel_y_dim) {
        _conv_kernels.resize(_input_dim, _output_dim);
		for (int i = 0; i < _input_dim;i ++) {
			for (int j = 0; j < _output_dim; j++) {	
				matrix_double kernel(_kernel_x_dim, _kernel_y_dim);
				kernel.assign_val();
				_conv_kernels[i][j].push_back(kernel);
			}
		}
        _bias.resize(1, _output_dim);
        _type = CONV;
    }
    
    void _set_conn_map(const matrix_int& conn_map) {
        _conn_map = conn_map;
    }

    void _forward(Layer* _pre_layer) {
        for (int i = 0; i < _output_dim; i++) {
            matrix_double feature_map(_pre_layer->_feature_x_dim - _kernel_x_dim + 1,
                _pre_layer->_feature_y_dim - _kernel_y_dim + 1);
            for (int j = 0; j < _input_dim; j++) {
                if (_conv_map[j][k]) {
                    feature_map = feature_map + _pre_layer._data[j].conv(_conv_kernels[i]);
                }
            }
            _data.push_back(sigmoid_m(feature_map + _bias[0][i]));
        }
    }

    void _backward(Layer* _nxt_layer) {
        if (_nxt_layer._type == POOL) {

            for (int i = 0; i < _output_dim; i++) {
                matrix_double error = (up_sampling(_nxt_layer->_errors[i])
                    .dot_mul(sigmoid_m_diff(_data[i]))) 
                    * _nxt_layer._pooling_weights[0][i]; 
                _errors.push_back(error);
            	
			}
        }
    }
}

class PoolingLayer : public Layer {

private:
    int _pooling_x_dim;
    int _pooling_y_dim;
    int _pooling_type; // 0 max_pooling 1 avg_pooling
    int _input_dim;
    int _output_dim;

public:
    matrix_double _pooling_weights;
    matrix_double _pooling_bias;
    virtual ~ PoolingLayer() {}
    PoolingLayer() {}

    PoolingLayer(int input_dim, int output_dim) :
        _input_dim(input_dim), _output_dim(output_dim) {
        _pooling_weights.resize(1, output_dim);
        _pooling_weights.assign_val();
        _pooling_bias.resize(1, output_dim);
        _pooling_bias.assign_val();
    }

    void _forward(Layer* _pre_layer) {
        for (int i = 0; i < _output_dim; i++) {
            matrix_double up_feature = _pre_layer->_data[i]
				.down_sample(_pooling_x_dim, _pooling_y_dim) * _pooling_weights[0][i] 
                + _pooling_bias[0][i];
            _data.push_back(sigmoid_m(up_feature));
        }
    }

    void _backward(Layer* _nxt_layer) {
        for (int i = 0; i < _output_dim; i++) {
            matrix_double error = _nxt_layer->_errors[i]
				.conv2(_nxt_layer->_conv_kernels[i], FULL)
				.dot_mul(sigmoid_m_diff(_data[i]));
            _erros.push_back(error);
        	_pooling_weights[0][i] = error.dot_mul(down_sample(_data[i])).sum();
		}
    }
};

class FullConnLayer : public Layer {

private:
    int _input_dim;
    int _output_dim;

public:
    matrix_double _full_conn_weights;
    matrix_double _full_conn_bias;
    matrix_double _delta_full_conn_weights;
    matrix_double _delta_full_conn_bias;

    virtual ~FullConnLayer() {}
    FullConnLayer() {}
    FullConnLayer(int intput_dim, int output_dim) {
        _full_conn_weights.resize(input_dim, output_dim);
        _full_conn_bias.resize(1, output_dim);
        _full_conn_weights.assign_val();
        _full_conn_bias.assign_val();
		_type = FULLCONN;
    }

    void _forward(Layer* _pre_layer) {
        _input_data = Matrix<double>::concat(_pre_layer->_data);
        _data.push_back(sigmoid_m(_input_data 
            * _full_conn_weights 
            + _full_conn_weights));
    }
    void _backward(Layer* _nxt_layer) {
		matrix_double error = _nxt_layer._erros[0] 
            * _nxt_layer._weights[0]._T()
            .dot_mul(sigmoid_m_diff(_data[0]));
        _errors.push_back(error);	
	}
};

class LossLayer : public Layer {
    
private:
    int _intput_dim;
    int _output_dim;
    matrix_double label;
    matrix_double _hidden_output_weights;
    matrix_double _output_bias;
    matrix_double _delta_hidden_output_weights;
    matrix_double _delta_output_bias;

public:
    virtual ~LossLayer() {}
    LossLayer(int input_dim, int output_dim) :
        _input_dim(input_dim), _output_dim(output_dim) {
        _hidden_output_weights.resize(input_dim, output_dim);
        _output_bias.resize(1, output_dim);
        _hidden_output_weights.assign_val();
        _output_bias.assign_val();
    }

    void _forward(Layer* _pre_layer) {
        _data.push_back(sigmoid_m(_pre_layer->_data[0] 
            * _hidden_output_weights 
            + _output_bias));
    }

    void _backward(Layer* _nxt_layer) {
        _errors.push_back((_data[0] - label)
            .dot_mul(sigmoid_m_diff(_data[0])));
		_delta_hidden_output_weights = _data[0]._T() * _errors[0];
		_delta_output_bias = _errors[0];
	}
};

void test_fullconnect() {
}

void test_cnn() {
}

int main() {

    return 0;
}