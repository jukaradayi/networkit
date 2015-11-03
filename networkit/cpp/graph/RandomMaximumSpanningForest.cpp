/*
 *
 */

#include "RandomMaximumSpanningForest.h"
#include "../auxiliary/SignalHandling.h"
#include "../auxiliary/Parallel.h"

namespace NetworKit {

RandomMaximumSpanningForest::RandomMaximumSpanningForest(const Graph &G) : G(G), hasWeightedEdges(false), hasMSF(false), hasAttribute(false) { };

template <typename A>
RandomMaximumSpanningForest::RandomMaximumSpanningForest(const Graph &G, const std::vector< A > &attribute) : G(G), hasWeightedEdges(false), hasMSF(false), hasAttribute(false) {
	if (!G.hasEdgeIds()) {
		throw std::runtime_error("Error: Edges of G must be indexed for using edge attributes");
	}

	weightedEdges.reserve(G.numberOfEdges());

	G.forEdges([&](node u, node v, edgeid eid) {
		weightedEdges.emplace_back(u, v, attribute[eid], eid);
	});

	INFO(weightedEdges.size(), " weighted edges saved");

	hasWeightedEdges = true;
}

// instantiate for count and edgeweight
template RandomMaximumSpanningForest::RandomMaximumSpanningForest<edgeweight>(const Graph &G, const std::vector<edgeweight>&);
template RandomMaximumSpanningForest::RandomMaximumSpanningForest<count>(const Graph &G, const std::vector<count>&);

void RandomMaximumSpanningForest::run() {
	hasRun = false;
	hasMSF = false;
	hasAttribute = false;

	Aux::SignalHandler handler;

	msf = G.copyNodes();

	handler.assureRunning();

	bool useEdgeWeights = false;

	if (!hasWeightedEdges) {
		weightedEdges.reserve(G.numberOfEdges());

		G.forEdges([&](node u, node v, edgeweight weight, edgeid eid) {
			weightedEdges.emplace_back(u, v, weight, eid);
		});

		hasWeightedEdges = true;
		useEdgeWeights = true;
	}

	handler.assureRunning();

	bool calculateAttribute = false;

	if (G.hasEdgeIds()) {
		msfAttribute.clear();
		msfAttribute.resize(G.upperEdgeIdBound(), false);
		calculateAttribute = true;
	}

	Aux::Parallel::sort(weightedEdges.begin(), weightedEdges.end(), std::greater<weightedEdge>());

	handler.assureRunning();

	UnionFind uf(G.upperNodeIdBound());

	for (weightedEdge e : weightedEdges) {
		if (uf.find(e.u) != uf.find(e.v)) {
			if (useEdgeWeights) {
				msf.addEdge(e.u, e.v, e.attribute);
			} else {
				msf.addEdge(e.u, e.v);
			}

			if (calculateAttribute) {
				msfAttribute[e.eid] = true;
			}

			uf.merge(e.u, e.v);
		}
	}

	handler.assureRunning();

	hasAttribute = calculateAttribute;
	hasMSF = true;
	hasRun = true;
}

bool RandomMaximumSpanningForest::inMSF(edgeid eid) const {
	if (!hasAttribute) throw std::runtime_error("Error: Either the attribute hasn't be calculated yet or the graph has no edge ids.");

	return msfAttribute[eid];
}

bool RandomMaximumSpanningForest::inMSF(node u, node v) const {
	if (hasMSF) {
		return msf.hasEdge(u, v);
	} else if (hasAttribute) {
		return msfAttribute[G.edgeId(u, v)];
	} else {
		throw std::runtime_error("Error: The run() method must be executed first");
	}
}

std::vector< bool > RandomMaximumSpanningForest::getAttribute(bool move) {
	std::vector<bool> result;

	if (!hasAttribute) throw std::runtime_error("Error: The run() method must be executed first");

	if (move) {
		result = std::move(msfAttribute);
		hasAttribute = false;
	} else {
		result = msfAttribute;
	}

	return result;
}

Graph RandomMaximumSpanningForest::getMSF(bool move) {
	Graph result;

	if (!hasMSF) throw std::runtime_error("Error: The run() method must be executed first");

	if (move) {
		result = std::move(msf);
		hasMSF = false;
	} else {
		result = msf;
	}

	return result;
}

std::string RandomMaximumSpanningForest::toString() const {
	return "Random maximum weight spanning forest";
}

bool RandomMaximumSpanningForest::isParallel() const {
	return false;
}


} // namespace NetworKit