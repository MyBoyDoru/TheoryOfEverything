#include <iostream>
#include <stdlib.h>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <cmath>

using namespace std;

class Neuron;

typedef vector<Neuron> Layer;

struct Connection
{
	double weight;
	double deltaWeight;
};

class Neuron 
{
public:
	Neuron(unsigned numOutputs, unsigned myIndex);
	void setOutputVal(double val) { m_outputVal = val; }
	double getOutputVal(void) const { return m_outputVal; }
	void feedForward(const Layer &prevLayer);
	void calcOutputGradients(double targetVal);
	void calcHiddenGradients(const Layer& nextLayer);
	void updateInputWeights(Layer& prevLayer);

private:
	static double randomWeight(void) { return rand() / double(RAND_MAX); }
	static double transferFunction(double x);
	static double transferFunctionDerivative(double x);
	double sumDOW(const Layer& nestLayer) const;
	double m_outputVal;
	vector<Connection> m_outputWeights;
	unsigned m_myIndex;
	double m_gradient;
	static double eta; // [0.0 ... 1.0]
	static double alpha; // [0.0 ... n]
};

double Neuron::eta = 0.15;
double Neuron::alpha = 0.5;

void Neuron::updateInputWeights(Layer& prevLayer)
{
	for (unsigned n = 0; n < prevLayer.size(); ++n)
	{
		Neuron& neuron = prevLayer[n];
		double oldDeltaWeight = neuron.m_outputWeights[m_myIndex].deltaWeight;

		double newDeltaWeight = eta * neuron.getOutputVal() * m_gradient + alpha * oldDeltaWeight;

		neuron.m_outputWeights[m_myIndex].deltaWeight = newDeltaWeight;
		neuron.m_outputWeights[m_myIndex].weight += newDeltaWeight;
	}
}

double Neuron::sumDOW(const Layer &nextLayer) const
{
	double sum = 0.0;

	//Sum the errors of the nodes

	for (unsigned n = 0; n < nextLayer.size()-1; ++n)
	{
		sum += m_outputWeights[n].weight * nextLayer[n] * m_gradient;
	}

	return sum;
}

void Neuron::calcHiddenGradients(const Layer& nextLayer)
{
	double dow = sumDOW(nextLayer);
	m_gradient = dow * Neuron::transferFunctionDerivative(m_outputVal);
}

void Neuron::calcOutputGradients(double targetVal)
{
	double delta = targetVal - m_outputVal;
	m_gradient = delta * Neuron::transferFunctionDerivative(m_outputVal);
}

double Neuron::transferFunction(double x)
{
	//tanh range [-1.0 ..... 1.0]
	return tanh(x);
}

double Neuron::transferFunctionDerivative(double x)
{
	//tanh derivative
	return 1.0 - x * x;
}

void Neuron::feedForward(const Layer& prevLayer)
{
	double sum = 0.0;
	//Sum the previous layers outputs
	//Include the bias node

	for (unsigned n = 0; n < prevLayer.size(); ++n)
	{
		sum += prevLayer[n].getOutputVal() *
			prevLayer[n].m_outputWeights[m_myIndex].weight;
	}

	m_outputVal = Neuron::transferFunction(sum);
}

Neuron::Neuron(unsigned numOutputs, unsigned myIndex)
{
	for (unsigned c = 0; c < numOutputs; ++c)
	{
		m_outputWeights.push_back(Connection());
		m_outputWeights.back().weight = randomWeight();
	}

	m_myIndex = myIndex;
}


class Net
{
public:
	Net(const vector <unsigned> &topology);
	void feedForward(const vector<double>& inputVals);
	void backProp(const vector<double>& targetVals);
	void getResults(vector<double>& resultVals) const;
private:
	vector <Layer> m_layers;
	double m_error;
	double m_recentAverageError;
	double m_recentAverageSmoothingFactor;
};

void Net::getResults(vector<double>& resultVals) const
{
	resultVals.clear();

	for (unsigned n = 0; n <= m_layers.back().size(); ++n)
	{
		resultVals.push_back(m_layers.back()[n].getOutputVal());
	}
}

void Net::backProp(const vector<double>& targetVals) 
{
	// Calculate overall error
	Layer& outputLayer = m_layers.back();
	m_error = 0.0;

	for (unsigned n = 0; n < outputLayer.size(); ++n)
	{
		double delta = targetVals[n] - outputLayer[n].getOutputVal();
		m_error += delta * delta;
	}
	m_error /= outputLayer.size() - 1; //Get average error squared
	m_error = sqrt(m_error); //RMS
	
	//Calculate average error
	m_recentAverageError =
		(m_recentAverageError * m_recentAverageSmoothingFactor + m_error) /
		(m_recentAverageSmoothingFactor + 1.0);

	//Calculate output layer gradients
	for (unsigned n = 0; n < outputLayer.size() - 1; ++n)
	{
		outputLayer[n].calcOutputGradients(targetVals[n]);
	}

	//Calculate gradients on hidden layers
	for (unsigned layerNum = m_layers.size() - 2; layerNum > 0; --layerNum)
	{
		Layer& hiddenLayer = m_layers[layerNum];
		Layer& nextLayer = m_layers[layerNum + 1];

		for (unsigned n = 0; n < hiddenLayer.size(); ++n)
		{
			hiddenLayer[n].calcHiddenGradients(nextLayer);
		}
	}

	//update connection weights
	for (unsigned layerNum = m_layers.size() - 1; layerNum > 0; --layerNum)
	{
		Layer& layer = m_layers[layerNum];
		Layer& prevLayer = m_layers[layerNum - 1];

		for (unsigned n = 0; n < layer.size(); ++n)
		{
			layer[n].updateInputWeights(prevLayer);
		}
	}
}
void Net::feedForward(const vector<double>& inputVals)
{
	assert(inputVals.size() == m_layers[0].size() - 1);

	//assign the input valuse onto the input neurons
	for (unsigned i = 0; i < inputVals.size(); ++i)
	{
		m_layers[0][i].setOutputVal(inputVals[i]);
	}

	//Forward propagate
	for (unsigned layerNum = 1 ;layerNum <m_layers.size();++layerNum)
		{
		Layer& prevLayer = m_layers[layerNum - 1];
		for (unsigned n = 0; n < m_layers[layerNum].size() -1; ++n)
			{
			m_layers[layerNum][n].feedForward(prevLayer);
			}
		}

}

Net::Net(const vector<unsigned> &topology)
{
	unsigned numLayers = topology.size();
	for (unsigned layerNum = 0; layerNum < numLayers; ++layerNum)
	{
		m_layers.push_back(Layer());
		unsigned numberOutputs = layerNum == topology.size() - 1 ? 0 : topology[layerNum + 1];

		//New layer created filled with neurons

		//Creating bias (auxiliary) neurons
		for (unsigned neuronNum = 0; neuronNum <= topology[layerNum]; ++neuronNum)
		{
			m_layers.back().push_back(Neuron(numberOutputs,neuronNum));
			cout << "Neuron Created!\n";
		}
	}
}



int main()
{
	vector <unsigned> topology;
	topology.push_back(3);
	topology.push_back(2);
	topology.push_back(1);
	Net myNet(topology);

	vector<double> inputVals;
	myNet.feedForward(inputVals);

	vector<double> targetVals;
	myNet.backProp(targetVals);

	vector<double> resultVals;
	myNet.getResults(resultVals);

	return 0;
}