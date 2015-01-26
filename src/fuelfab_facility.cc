#include "fuelfab_facility.h"

namespace fuelfab {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    FuelfabFacility::FuelfabFacility(cyclus::Context* ctx)
        : cyclus::Facility(ctx) {};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    std::string FuelfabFacility::str() {
      return Facility::str();
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void FuelfabFacility::Tick() {
        if(inventory.size() == 0){
            cyclus::toolkit::ResourceBuff resource;
            for(int i = 0; i < in_commods.size(); i++){
                inventory.push_back(resource);
            }
        }//outputs whats inside the facility
        //std::cout << "FuelFab Inventory:" << std::endl;
        for(int i = 0; i < inventory.size(); i++){
            //sets max inventory at startup
            if(inventory[i].count() == 0){
                inventory[i].set_capacity(maximum_storage/inventory.size());
            }

            //std::cout << "  " << i+1 << ": " << inventory[i].quantity() << std::endl;
            /*if(inventory[i].count() != 0){
                std::vector<cyclus::Material::Ptr> manifest;
                manifest = cyclus::ResCast<cyclus::Material>(inventory[i].PopN(inventory[i].count()));

                cyclus::CompMap comp;
                cyclus::CompMap::iterator it;

                comp = manifest[0]->comp()->mass();

                inventory[i].PushAll(manifest);
            }*/
        }
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void FuelfabFacility::Tock() {

    }

    std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr> FuelfabFacility::GetMatlRequests() {

         //std::cout << "ff GetMatreq" << std::endl;
        using cyclus::RequestPortfolio;
        using cyclus::Material;
        using cyclus::Composition;
        using cyclus::CompMap;
        using cyclus::CapacityConstraint;
        std::set<RequestPortfolio<Material>::Ptr> ports;
        cyclus::Context* ctx = context();
        CompMap cm;
        Material::Ptr target = Material::CreateUntracked(maximum_storage/inventory.size(), Composition::CreateFromAtom(cm));
        RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());

        double qty = fissle_inv.space();
        port->AddRequest(target, this, fissle_stream);
        CapacityConstraint<Material> cc(qty);
        port->AddConstraint(cc);
        ports.insert(port);

        RequestPortfolio<Material>::Ptr port2(new RequestPortfolio<Material>());
        qty = non_fissle_inv.space();
        port2->AddRequest(target, this, non_fissle_stream);
        CapacityConstraint<Material> cc2(qty);
        port2->AddConstraint(cc2);
        ports.insert(port2);

        std::map<std::string, double>::iterator i;
        int it = 0;
        for(i = in_commods.begin(); i != in_commods.end(); ++i){
            qty = inventory[it].space();
            RequestPortfolio<Material>::Ptr port3(new RequestPortfolio<Material>());
            //std::cout << "FF _ QTY " << qty << std::endl;
            port3->AddRequest(target, this, i->first);
            CapacityConstraint<Material> cc3(qty);
            port3->AddConstraint(cc);
            ports.insert(port3);
            it++;
        }
        //std::cout << "ff GetMatreq end" << std::endl;
        return ports;
    }

    // MatlBids //
    std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>FuelfabFacility::GetMatlBids(cyclus::CommodMap<cyclus::Material>::type& commod_requests) {
    using cyclus::BidPortfolio;
    using cyclus::CapacityConstraint;
    using cyclus::Converter;
    using cyclus::Material;
    using cyclus::Request;
    using reactor::ReactorFacility;
    cyclus::Context* ctx = context();
    std::set<BidPortfolio<Material>::Ptr> ports;
    //std::cout << "FF matbid" << std::endl;
    // respond to all requests of my commodity
    int inventory_test = 0;
    if(fissle_inv.count() > 0 && non_fissle_inv.count() > 0 ){
        inventory_test += 1;
    }
    for(int i = 0; i < inventory.size(); i++){
        if(inventory[i].count() > 0){
            inventory_test += 1;
        }
    }
    CapacityConstraint<Material> cc(1);
    if (inventory_test == 0){return ports;}
    std::vector<Request<Material>*>& requests = commod_requests[out_commod];
    std::vector<Request<Material>*>::iterator it;
    std::map<cyclus::Trader*, int> facility_request;
    Request<Material>* req;
    //Set up a map for facility requestor to number of requests
    for (it = requests.begin(); it != requests.end(); ++it) {
        Request<Material>* req = *it;
        facility_request[req->requester()] += 1;
    }
    //Iterate through requesters
    std::map<cyclus::Trader*, int>::iterator id;
    for(id = facility_request.begin(); id != facility_request.end(); ++id){
        double limit, nlimit;
        //Cast requester as a reactor if possible
        ReactorFacility* reactor = dynamic_cast<ReactorFacility*>(id->first);
        if (!reactor){
           throw cyclus::CastError("No reactor for fuelfab facility.");
        } else {
            // Check for reactor start up
            if (reactor->inventory.count() == 0){
                limit = reactor->start_up(fissle_inv, non_fissle_inv, inventory, in_commods);
                double temp_limit = limit;
                int k = 1;
                //For each request from reactor create a trade portfolio
                for(it = requests.begin(); it!=requests.end(); ++it){
                    Request<Material>* req = *it;
                    if(req->requester() == id->first){
                        BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
                        limit = temp_limit*k/(reactor->batches);
                        nlimit = reactor->core_mass/reactor->batches-limit;
                        double qty = limit + nlimit;
                        qty = qty <= 0 ? 1 : qty;
                        CapacityConstraint<Material> cc(qty);
                        cyclus::Material::Ptr manifest;
                        manifest = cyclus::ResCast<Material>(fissle_inv.Pop());
                        Material::Ptr offer = Material::CreateUntracked(limit, manifest->comp());
                        fissle_inv.Push(manifest);
                        manifest = cyclus::ResCast<Material>(non_fissle_inv.Pop());
                        offer->Absorb(Material::CreateUntracked(nlimit, manifest->comp()));
                        non_fissle_inv.Push(manifest);
                        port->AddBid(req, offer, this);
                        port->AddConstraint(cc);
                        ports.insert(port);
                        k++;
                    }
                }
                //Same for refueling
            } else{
                for(it = requests.begin(); it!=requests.end(); ++it){
                    Request<Material>* req = *it;
                    if(req->requester() == id->first){
                        BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
                        limit = reactor->blend_next(fissle_inv, non_fissle_inv, inventory, in_commods);
                        nlimit = reactor->core_mass/reactor->batches-limit;
                        double qty = limit + nlimit;
                        qty = qty <= 0 ? 1 : qty;
                        CapacityConstraint<Material> cc(qty);
                        cyclus::Material::Ptr manifest;
                        manifest = cyclus::ResCast<Material>(fissle_inv.Pop());
                        Material::Ptr offer = Material::CreateUntracked(limit, manifest->comp());
                        fissle_inv.Push(manifest);
                        manifest = cyclus::ResCast<Material>(non_fissle_inv.Pop());
                        offer->Absorb(Material::CreateUntracked(nlimit, manifest->comp()));
                        non_fissle_inv.Push(manifest);
                        port->AddBid(req, offer, this);
                        port->AddConstraint(cc);
                        ports.insert(port);
                        }
                    }
                }
            }
        }
        //std::cout << "FF matbid end" << std::endl;
        return ports;
    }

    void FuelfabFacility::AcceptMatlTrades(const std::vector< std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >& responses) {
        //std::cout << "ff TRADE start" << std::endl;
        std::vector<std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >::const_iterator it;
        for (it = responses.begin(); it != responses.end(); ++it) {
            std::map<std::string, double>::iterator i;
            double k = 0;
            for(i = in_commods.begin(); i != in_commods.end(); ++i){
                if(it->first.request->commodity() == i->first){
                    inventory[k].Push(it->second);
                }
                k++;
            }
            if(it->first.request->commodity() == fissle_stream){
                fissle_inv.Push(it->second);
            }
            if(it->first.request->commodity() == non_fissle_stream){
                non_fissle_inv.Push(it->second);
            }
        }
        //std::cout << "ff TRADE end" << std::endl;
    }

    void FuelfabFacility::GetMatlTrades(const std::vector< cyclus::Trade<cyclus::Material> >& trades,
                                        std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                        cyclus::Material::Ptr> >& responses) {
        using cyclus::Material;
        using cyclus::Trade;
        std::map<cyclus::Trader*, int> facility_request;
        std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
        std::vector<cyclus::Material::Ptr> manifest;
        //std::cout << "ffgetTRADE start" << std::endl;
        //Setting up comp map of requesters to number of requests
        for(it = trades.begin(); it != trades.end(); ++it){
            cyclus::Request<Material> req = *it->request;
            facility_request[req.requester()] += 1;
        }
        std::map<cyclus::Trader*, int>::iterator id;
        //iterate through requests
        for(id = facility_request.begin(); id != facility_request.end(); ++id){
            double limit, nlimit;
            //cast requester as reactor
            reactor::ReactorFacility* reactor = dynamic_cast<reactor::ReactorFacility*>(id->first);
            if (!reactor){
               throw cyclus::CastError("No reactor for fuelfab facility.");
            } else {
                //start up
                if (reactor->inventory.count() == 0){
                    limit = reactor->start_up(fissle_inv, non_fissle_inv, inventory, in_commods);
                    double temp_limit = limit;
                    int k = 0;
                    for(it = trades.begin(); it!=trades.end(); ++it){
                        cyclus::Request<Material> req = *it->request;
                        if(req.requester() == id->first){
                            limit = temp_limit/2*(1+(k/(reactor->batches)));
                            nlimit = reactor->core_mass/reactor->batches-limit;
                            manifest = cyclus::ResCast<Material>(fissle_inv.PopQty(limit));
                            Material::Ptr offer = manifest[0]->ExtractComp(0., manifest[0]->comp());
                            for(int i = 0; i < manifest.size(); i++){
                                offer->Absorb(manifest[i]->ExtractComp(manifest[i]->quantity(), manifest[i]->comp()));
                            }
                            manifest = cyclus::ResCast<Material>(non_fissle_inv.PopQty(nlimit));
                            for(int i = 0; i < manifest.size(); i++){
                                offer->Absorb(manifest[i]->ExtractComp(manifest[i]->quantity(), manifest[i]->comp()));
                            }
                            std::map<std::string, double>::iterator md;
                            int k = 0;
                            for(md = in_commods.begin(); md != in_commods.end(); ++md){
                                manifest = cyclus::ResCast<Material>(inventory[k].PopQty(reactor->core_mass/reactor->batches * md->second));
                                for(int i = 0; i < manifest.size(); i++){
                                    offer->Absorb(manifest[i]->ExtractComp(manifest[i]->quantity(), manifest[i]->comp()));
                                }
                                k++;
                            }
                            responses.push_back(std::make_pair(*it, offer));
                            k++;
                        }
                    }
                } else{
                    //non-start up refueling
                    for(it = trades.begin(); it!=trades.end(); ++it){
                        cyclus::Request<Material> req = *it->request;
                        if(req.requester() == id->first){
                            limit = reactor->blend_next(fissle_inv, non_fissle_inv, inventory, in_commods);
                            nlimit = reactor->core_mass/reactor->batches-limit;
                            manifest = cyclus::ResCast<Material>(fissle_inv.PopQty(limit));
                            Material::Ptr offer = manifest[0]->ExtractComp(0., manifest[0]->comp());
                            for(int i = 0; i < manifest.size(); i++){
                                offer->Absorb(manifest[i]->ExtractComp(manifest[i]->quantity(), manifest[i]->comp()));
                            }
                            manifest = cyclus::ResCast<Material>(non_fissle_inv.PopQty(nlimit));
                            for(int i = 0; i < manifest.size(); i++){
                                offer->Absorb(manifest[i]->ExtractComp(manifest[i]->quantity(), manifest[i]->comp()));
                            }
                            std::map<std::string, double>::iterator md;
                            int k = 0;
                            for(md = in_commods.begin(); md != in_commods.end(); ++md){
                                manifest = cyclus::ResCast<Material>(inventory[k].PopQty(reactor->core_mass/reactor->batches * md->second));
                                for(int i = 0; i < manifest.size(); i++){
                                    offer->Absorb(manifest[i]->ExtractComp(manifest[i]->quantity(), manifest[i]->comp()));
                                }
                                k++;
                            }
                            responses.push_back(std::make_pair(*it, offer));
                        }
                    }
                }
            }
        }
        //std::cout << "ff getTRADE end" << std::endl;
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    extern "C" cyclus::Agent* ConstructFuelfabFacility(cyclus::Context* ctx) {
        return new FuelfabFacility(ctx);
    }

}
