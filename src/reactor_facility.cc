#include "reactor_facility.h"

namespace reactor {

ReactorFacility::ReactorFacility(cyclus::Context* ctx)
    : cyclus::Facility(ctx) {
      cycle_end_ = ctx->time() +1;
      start_time_ = cycle_end_;
      shutdown = false;
      refuels = 0;
      record = true;
};

std::string ReactorFacility::str() {
  return Facility::str();
}

void CompOut(cyclus::Material::Ptr mat1){
    cyclus::CompMap comp;
    comp = mat1->comp()->mass(); //store the fractions of i'th batch in comp
    int comp_iso;
    cyclus::CompMap::iterator it;
    //each iso in comp
    for (it = comp.begin(); it != comp.end(); ++it){
        std::cout<<it->first<< " " << it->second << std::endl;
    }
}

void ReactorFacility::Tick() {
    //std::cout << "reactorfacility inventory size: " << inventory.count() << std::endl;
    if(shutdown == true){return;}
    if(fuel_library_.name.size() == 0){
        std::cout << "New " << libraries[0] << " reactor starting up - target burnup = " << target_burnup << std::endl;
        std::string manifest_file = cyclus::Env::GetInstallPath() + "/share/brightlite/" + \
                          libraries[0] + "/manifest.txt";
        //std::cout << manifest_file << std::endl;
        std::ifstream inf(cyclus::Env::GetInstallPath() + "/share/brightlite/" + \
                          libraries[0] + "/manifest.txt"); //opens manifest file
        std::string line;
        std::string iso_name;
        fuel_library_.name = libraries[0]; //for now only one entry in here
        while(getline(inf, line)){
            std::istringstream iss(line);
            isoInformation iso; //creates temporary iso
            iss >> iso_name;
            iso.name = atoi(iso_name.c_str()); //adds name to iso
            iso.region = 0; //default region is the zeroth region
            iso.fraction = 0; //zero fraction
            fuel_library_.all_iso.push_back(iso); //adds this temp iso to fuel_library
        }
        if (libraries.size() < 2){
        //adds a built isoInfo for each isotope that can be in fuel, stores it in all_iso
            DataReader2(fuel_library_.name, fuel_library_.all_iso);
        } else {
            for(int i = 0; i < libraries.size(); i++){
                fuel_library_.interpol_libs.push_back(libraries[i]);
            }
            for(std::map<std::string, double>::iterator c = interpol_pairs.begin(); c != interpol_pairs.end(); ++c){
                interpol_pair pair;
                pair.metric = c -> first;
                pair.value = c -> second;
                fuel_library_.interpol_pairs.push_back(pair);
            }
            fuel_library_ = lib_interpol(fuel_library_);
        }
        //adds general info about the fuel in fuel_library_
        ///if theres value, dont update field
        fuel_library_.name = libraries[0];
        fuel_library_.base_flux = flux_finder(fuel_library_.name);
        fuel_library_.base_mass = core_mass;
        fuel_library_.base_power = generated_power;
        fuel_library_.pnl = nonleakage;
        fuel_library_.target_BU = target_burnup;
        fuel_library_.fuel_area = fuel_area;
        fuel_library_.cylindrical_delta = cylindrical_delta;
        fuel_library_.mod_Sig_a = mod_Sig_a;
        fuel_library_.mod_Sig_tr = mod_Sig_tr;
        fuel_library_.mod_Sig_f = mod_Sig_f;
        fuel_library_.mod_thickness = mod_thickness;
        fuel_library_.fuel_Sig_tr = fuel_Sig_tr;
        fuel_library_.disadv_a = disadv_a;
        fuel_library_.disadv_b = disadv_b;
        fuel_library_.disadv_mod_siga = disadv_mod_siga;
        fuel_library_.disadv_mod_sigs = disadv_mod_sigs;
        fuel_library_.disadv_fuel_sigs = disadv_fuel_sigs;


        batch_info empty_batch;
        //creates empty batch entries
        for(int i = 0; i < batches; i++){
            fuel_library_.batch.push_back(empty_batch);
            fuel_library_.batch[i].batch_fluence = 0; //fluence of the batch is set to zero
            fuel_library_.batch[i].fraction.push_back(1);//fraction of the batch is set to one
        }/*
        core_ = std::vector<fuelBundle>(batches); //core_ is size of batches, which will be changed
        for(int i = 0; i < core_.size(); i++){
          core_[i] = fuel_library_[i];
        }*/

/************************output file*********************************/
        std::ofstream outfile("../output_cyclus_recent.txt");

        outfile << "Fuel library name: " << fuel_library_.name << "\r\n";
        outfile << "Batches: " << batches << "\r\n";
        outfile << "libcheck: " << fuel_library_.libcheck << "\r\n";
        outfile << "Leakage: " << fuel_library_.pnl << "\r\n";
        outfile << "Burnup calculation timestep: " << burnupcalc_timestep << " [days]\r\n";
        outfile << "Flux calculation method: " << flux_mode << "\r\n";
        if(flux_mode == 3){//if the flux calculation is the cylindrical method
        outfile << "   Fuel area: " << fuel_library_.fuel_area << " [cm2]\r\n";
        outfile << "   Fuel Sig_tr: " << fuel_library_.fuel_Sig_tr << " [cm-1]\r\n";
        outfile << "   Delta: " << fuel_library_.cylindrical_delta << " [cm]\r\n";
        outfile << "   Moderator thickness: " << fuel_library_.mod_thickness << " [cm]\r\n";
        outfile << "   Moderator Sig_a: " << fuel_library_.mod_Sig_a << " [cm-1]\r\n";
        outfile << "   Moderator Sig_tr: " << fuel_library_.mod_Sig_tr << " [cm-1]\r\n";
        outfile << "   Moderator Sig_f: " << fuel_library_.mod_Sig_f << " [cm-1]\r\n";
        }
        outfile << "\r\n|Thermal disadvantage calculation:\r\n";
        outfile << "| Fuel radius: " << fuel_library_.disadv_a << " [cm]\r\n";
        outfile << "| Moderator radius: " << fuel_library_.disadv_b << " [cm]\r\n";
        outfile << "| Moderator Sigma_a: " << fuel_library_.disadv_mod_siga << " [cm-1]\r\n";
        outfile << "| Moderator Sigma_s: " << fuel_library_.disadv_mod_sigs << " [cm-1]\r\n";
        outfile << "| Fuel Sigma_s: " << fuel_library_.disadv_fuel_sigs << " [cm-1]\r\n\r\n";
        outfile << "Base flux: " << fuel_library_.base_flux << "\r\n";
        outfile << "Base power: " << fuel_library_.base_power << "\r\n";
        outfile << "Base mass: " << fuel_library_.base_mass << "\r\n\r\n\r\n";

        outfile.close();
/************************End of output file*********************************/
    }



    //std::cout << "end tick" << std::endl;
}

void ReactorFacility::Tock() {
    if(inventory.count() == 0){return;}
    if(shutdown == true){return;}

    cyclus::Context* ctx = context();
    if (ctx->time() != cycle_end_) {
        //std::cout << "time: "<< ctx->time()<< "  not end of cycle.  End of cycle: " << cycle_end_ << std::endl;/// <--------
        return;
    }

    // Pop materials out of inventory
    std::vector<cyclus::Material::Ptr> manifest;
    manifest = cyclus::ResCast<cyclus::Material>(inventory.PopN(inventory.count()));

    cyclus::CompMap comp;
    cyclus::CompMap::iterator it;
    if(manifest.size() > fuel_library_.batch.size()){
        for(int i = 0; i < manifest.size() - fuel_library_.batch.size(); i++){
            batch_info temp_batch;
            temp_batch.batch_fluence = 0;
            fuel_library_.batch.push_back(temp_batch);
        }
    }
    //each batch
    for(int i = 0; i < manifest.size(); i++){
    //build correct isoinfo and fraction for each batch
    ///put the stuff in comp fraction to fuel_library_. .iso fraction using values in fuel_library_.all_iso
        comp = manifest[i]->comp()->mass(); //store the fractions of i'th batch in comp
        int comp_iso;
        //each iso in comp
        for (it = comp.begin(); it != comp.end(); ++it){
            comp_iso = pyne::nucname::zzaaam(it->first);
            //std::cout << "Isotope " << it->first << " amount " << it->second << std::endl;
            //each iso in all_iso
            for(int j = 0; j < fuel_library_.all_iso.size(); j++){
                int fl_iso = fuel_library_.all_iso[j].name;
                if(fl_iso == comp_iso && fuel_library_.batch[i].batch_fluence == 0){
                    //std::cout << "i: " << i << "  " << fl_iso << "  " << comp_iso << "   "<<  it->second << std::endl;
                    isoInformation temp_iso;
                    temp_iso = fuel_library_.all_iso[j];
                    temp_iso.fraction = it->second/(core_mass/batches);
                    fuel_library_.batch[i].iso.push_back(temp_iso);
                }
            }
        }
    }
    //collapse iso's, read struct effects, reorder the fuel batches accordingly
    batch_reorder();

    /*for(int i = 0; i < fuel_library_.batch.size(); i++){
        std::cout << "batch u235 frac: " << fuel_library_.batch[i].comp[922350] << "  " << fuel_library_.batch[i].collapsed_iso.neutron_prod[0] << std::endl;
    }*/

      // pass fuel bundles to burn-up calc
    fuel_library_ = burnupcalc(fuel_library_, flux_mode, DA_mode, burnupcalc_timestep);

        //temp ss burnupcalc test
        //std::cout << "SS burnup: " << SS_burnupcalc(fuel_library_.batch[0].collapsed_iso, 1, 20, 0.99, 3E14) << std::endl;


    //end temp test


  // convert fuel bundle into materials
    for(int i = 0; i < fuel_library_.batch.size(); i++){
        cyclus::CompMap out_comp;
        for(std::map<int, double>::iterator c = fuel_library_.batch[i].comp.begin(); c != fuel_library_.batch[i].comp.end(); ++c){
            if(c->second < 0){
            out_comp[pyne::nucname::zzaaam_to_id(c->first)] = 0;
        } else {
            out_comp[pyne::nucname::zzaaam_to_id(c->first)] = c->second;
        }
    }
    manifest[i]->Transmute(cyclus::Composition::CreateFromMass(out_comp));
    inventory.Push(manifest[i]);
  }

  // cycle end update
  cycle_end_ = ctx->time() + ceil(fuel_library_.batch[fuel_library_.batch.size()-1].batch_fluence/(86400*fuel_library_.base_flux*28));
  refuels += 1;


  if(ctx->time() > start_time_ + reactor_life && record == true){
    //its time to shut down


    shutdown = true;

     for(int i = 0; i < fuel_library_.batch.size(); i++){
        int ii;
        double burnup;
        for(ii = 0; fuel_library_.batch[i].collapsed_iso.fluence[ii] < fuel_library_.batch[i].batch_fluence; ii++){}
        burnup = intpol(fuel_library_.batch[i].collapsed_iso.BU[ii-1], fuel_library_.batch[i].collapsed_iso.BU[ii], fuel_library_.batch[i].collapsed_iso.fluence[ii-1], fuel_library_.batch[i].collapsed_iso.fluence[ii], fuel_library_.batch[i].batch_fluence);
        //std::cout << "Reactor " << libraries[0] << " burnup at shutdown " << burnup << std::endl;
        context()->NewDatum("BrightLite_Reactor_Data")
        ->AddVal("AgentID", id())
        ->AddVal("Time", cycle_end_)
        ->AddVal("Discharge_Fluence", burnup)
        ->AddVal("Batch_No", std::to_string(refuels+i+1))
        ->Record();

     }
    record = false;
    //cycle_end_ = 9999;//ctx->cyclus::SimInfo::duration;
  }


    /************************output file*********************************/
    std::ofstream outfile;
    outfile.open("../output_cyclus_recent.txt", std::ios::app);

    outfile << " Cycle length: " << ceil(fuel_library_.batch[fuel_library_.batch.size()-1].batch_fluence/(86400*fuel_library_.base_flux*28)) << " [months]";

    outfile << "\r\n\r\n\r\n";

    outfile.close();
    /************************End of output file**************************/

  if(shutdown != true && record == true){
      std::cout << "BURNUP: " << fuel_library_.batch[0].discharge_BU << std::endl;
      //add batch variable to cyclus database
      ///time may need to be fixed by adding cycle length to it
      context()->NewDatum("BrightLite_Reactor_Data")
               ->AddVal("AgentID", id())
               ->AddVal("Time", cycle_end_)
               ->AddVal("Discharge_Burnup", fuel_library_.batch[0].discharge_BU)
               ->AddVal("Batch_No", std::to_string(refuels))
               //->AddVal("Discharge_Fluence", fuel_library_.batch[0].batch_fluence)
               /*->AddVal("Next Cycle Length", ceil(fuel_library_.batch[fuel_library_.batch.size()-1].batch_fluence/(86400*fuel_library_.base_flux*28)))
               ->AddVal("Discharge_U", fuel_library_.batch[0].comp[922340000]+fuel_library_.batch[0].comp[922350000]+fuel_library_.batch[0].comp[922360]+fuel_library_.batch[0].comp[922370]+fuel_library_.batch[0].comp[922380])
               ->AddVal("Discharge_U235", fuel_library_.batch[0].comp[922350])
               ->AddVal("Discharge_U238", fuel_library_.batch[0].comp[922380])
               ->AddVal("Discharge_PU", fuel_library_.batch[0].comp[942380]+fuel_library_.batch[0].comp[942390]+fuel_library_.batch[0].comp[942400]+fuel_library_.batch[0].comp[942410]+fuel_library_.batch[0].comp[942420])
               ->AddVal("Discharge_PU239", fuel_library_.batch[0].comp[942390])
               ->AddVal("Discharge_PU241", fuel_library_.batch[0].comp[942410])*/
               ->Record();
   }
}


std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr> ReactorFacility::GetMatlRequests() {
    //std::cout << "Getmatlrequests begin" << std::endl;
    using cyclus::RequestPortfolio;
    using cyclus::Material;
    using cyclus::Composition;
    using cyclus::CompMap;
    using cyclus::CapacityConstraint;
    std::set<RequestPortfolio<Material>::Ptr> ports;
    cyclus::Context* ctx = context();
    if (ctx->time() != cycle_end_ && inventory.count() != 0 || shutdown == true){return ports;}

    CompMap cm;
    Material::Ptr target = Material::CreateUntracked(core_mass/batches, Composition::CreateFromAtom(cm));

    RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
    double qty;

    if(inventory.count() == 0){
        if(target_burnup == 0){
            for(int i = 0; i < batches; i++){
            //checks to see if there is a next in_commod to request, otherwise puts the first commod request
                if(in_commods.size() > i+1){
                    port->AddRequest(target, this, in_commods[i+1]);
                } else {
                    port->AddRequest(target, this, in_commods[0]);
                }
            }
        } else {
            for(int i = 0; i < batches; i++){
                port->AddRequest(target, this, in_commods[0]);
            }
        }

        qty = core_mass;
    } else {
        port->AddRequest(target, this, in_commods[0]);
        qty = core_mass/batches;
    }
    //std::cout << "RX Request " << qty << std::endl;
    CapacityConstraint<Material> cc(qty);

    port->AddConstraint(cc);
    ports.insert(port);
    //std::cout << "end getmatlrequests" << std::endl;
    return ports;
}

// MatlBids //
std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr> ReactorFacility::GetMatlBids(
    cyclus::CommodMap<cyclus::Material>::type& commod_requests) {
  //std::cout << "RX GetMatBid" << std::endl;
  using cyclus::BidPortfolio;
  using cyclus::CapacityConstraint;
  using cyclus::Converter;
  using cyclus::Material;
  using cyclus::Request;
  cyclus::Context* ctx = context();
  std::set<BidPortfolio<Material>::Ptr> ports;

  //if its not the end of a cycle dont get rid of your fuel
  if (ctx->time() != cycle_end_){
    return ports;
  }

  //if theres nothing to give dont offer anything..
  if (inventory.count() == 0){return ports;}


  std::vector<cyclus::Material::Ptr> manifest;
  manifest = cyclus::ResCast<Material>(inventory.PopN(inventory.count()));
  BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
  std::vector<Request<Material>*>& requests = commod_requests[out_commod];
  std::vector<Request<Material>*>::iterator it;
  for (it = requests.begin(); it != requests.end(); ++it) {
    Request<Material>* req = *it;
    if (req->commodity() == out_commod) {
        if(shutdown == true){
            for(int i = 0; i < manifest.size(); i++){
                Material::Ptr offer = Material::CreateUntracked(core_mass/batches, manifest[i]->comp());
                port->AddBid(req, offer, this);
            }
        } else{
            Material::Ptr offer = Material::CreateUntracked(core_mass/batches, manifest[0]->comp());
            port->AddBid(req, offer, this);
        }

    }
  }
  inventory.PushAll(manifest);

  /*if(shutdown != true){
    CapacityConstraint<Material> cc(core_mass/batches);
    port->AddConstraint(cc);
  }*/

  ports.insert(port);
  //std::cout << "end getmatlbids" << std::endl;
  return ports;
}

void ReactorFacility::AcceptMatlTrades(const std::vector< std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >& responses) {
    //std::cout << "RX TRADE START" << std::endl;
    if(shutdown != true){
        std::vector<std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >::const_iterator it;
        cyclus::Composition::Ptr compost;
        //Initital core loading setup.
        if(target_burnup == 0){
            for (it = responses.begin(); it != responses.end(); ++it) {
                inventory.Push(it->second);
                compost = it->second->comp();
                cyclus::CompMap cmap = compost->mass();
                cyclus::CompMap::iterator cit;

        /************************output file*********************************/
                std::ofstream outfile;
                outfile.open("../output_cyclus_recent.txt", std::ios::app);
                outfile << "Composition of fresh batch:\r\n";

                for(cit = cmap.begin(); cit != cmap.end(); ++cit){
                    outfile << "  Isotope: " << cit->first << "  Fraction: " << cit->second << "\r\n";
                }
                outfile << "\r\n\n";

        /************************End of output file**************************/
          }
      } else {
        //Operational reloading
        for (it = responses.begin(); it != responses.end(); ++it) {
            if(it->first.request->commodity() == in_commods[0]){
                //CompOut(it->second);
                inventory.Push(it->second);
            }
        }
      }
  }
  //std::cout << "RX TRADE END" << std::endl;
}

void ReactorFacility::GetMatlTrades(const std::vector< cyclus::Trade<cyclus::Material> >& trades,
    std::vector<std::pair<cyclus::Trade<cyclus::Material>,cyclus::Material::Ptr> >& responses) {
    using cyclus::Material;
    using cyclus::Trade;
    //std::cout << "RX getTRADE START" << std::endl;
    std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
    //Remove the core loading
    if(shutdown == true){
        std::vector<cyclus::Material::Ptr> discharge = cyclus::ResCast<Material>(inventory.PopN(inventory.count()));
        fuel_library_.batch.clear();
        int i = 0;
        for (it = trades.begin(); it != trades.end(); ++it) {
            responses.push_back(std::make_pair(*it, discharge[i]));
            i++;
        }
    }else{
        //Remove the last batch from the core.
        cyclus::Material::Ptr discharge = cyclus::ResCast<Material>(inventory.Pop());
        fuel_library_.batch.erase(fuel_library_.batch.begin());
        for (it = trades.begin(); it != trades.end(); ++it) {
            responses.push_back(std::make_pair(*it, discharge));
        }
    }
    //std::cout << "RX getTRADE end" << std::endl;

}

fuelBundle ReactorFacility::comp_function(cyclus::Material::Ptr mat1, fuelBundle fuel_library_){

    cyclus::CompMap comp;
    cyclus::CompMap::iterator it;
    comp = mat1->comp()->mass(); //store the fractions of i'th batch in comp

    int comp_iso;
    fuelBundle temp_bundle = fuel_library_;
    batch_info info;
    temp_bundle.batch.clear();

    temp_bundle.batch.push_back(info);
    temp_bundle.batch[0].batch_fluence = 0;
    temp_bundle.batch[0].fraction.push_back(1);//fraction of the batch is set to one

    //each iso in comp
    for (it = comp.begin(); it != comp.end(); ++it){
        //std::cout<< it->first << ": " << it->second << std::endl;
        comp_iso = pyne::nucname::zzaaam(it->first);
        //each iso in all_iso
        for(int j = 0; j < temp_bundle.all_iso.size(); j++){
            int fl_iso = temp_bundle.all_iso[j].name;
            if(fl_iso == comp_iso && temp_bundle.batch[0].batch_fluence == 0){
                isoInformation temp_iso;
                temp_iso = temp_bundle.all_iso[j];
                temp_iso.fraction = it->second;
                std::cout << "Name: " << it->first << "Amount " << it->second << std::endl;
                temp_bundle.batch[0].iso.push_back(temp_iso);
            }
        }
    }
    temp_bundle = StructReader(temp_bundle);
    temp_bundle = regionCollapse(temp_bundle);
    for(int i = 0; i < temp_bundle.batch.size(); i++){
        temp_bundle.batch[i].Fg = 0;
    }

    return temp_bundle;
}

fuelBundle ReactorFacility::comp_trans(cyclus::Material::Ptr mat1, fuelBundle fuel_library_){
    //unlike the comp_function, this function assumes the first entry batch will be discharged
    //it replaces the last batch with the fraction coming from mat
    cyclus::CompMap comp;
    cyclus::CompMap::iterator it;
    comp = mat1->comp()->mass(); //store the fractions of i'th batch in comp

    int comp_iso;
    fuelBundle temp_bundle = fuel_library_;
    batch_info info;
    temp_bundle.batch.erase(temp_bundle.batch.begin());
    info.batch_fluence = 0;
    info.Fg = 0;
    info.fraction.push_back(1);

    temp_bundle.batch.push_back(info);

    //each iso in comp
    for (it = comp.begin(); it != comp.end(); ++it){
        //std::cout<< it->first << ": " << it->second << std::endl;
        comp_iso = pyne::nucname::zzaaam(it->first);
        //each iso in all_iso
        for(int j = 0; j < temp_bundle.all_iso.size(); j++){
            int fl_iso = temp_bundle.all_iso[j].name;
            if(fl_iso == comp_iso){
                isoInformation temp_iso;
                temp_iso = temp_bundle.all_iso[j];
                temp_iso.fraction = it->second;
                std::cout << "Name: " << it->first << "Amount " << it->second << std::endl;
                temp_bundle.batch[temp_bundle.batch.size()-1].iso.push_back(temp_iso);
            }
        }
    }
    temp_bundle = StructReader(temp_bundle);
    //std::cout << temp_bundle.batch.size() << std::endl;
    temp_bundle = regionCollapse(temp_bundle);
    return temp_bundle;
}

double ReactorFacility::blend_next(cyclus::toolkit::ResourceBuff fissle,
                                   cyclus::toolkit::ResourceBuff non_fissle,
                                   std::vector<cyclus::toolkit::ResourceBuff> inventory,
                                   std::map<std::string, double> incommods){
    double return_amount;//function return the amount of first stream in inventory to reach target burnup
    double return_prev = SS_enrich;
    double mass_frac = 1.;
    //turn inventory to materials
    //Read the fuelfab inventory
    if(refuels < batches){
        return SS_enrich;
        //burnup_target = target_burnup/(batches+1)*(refuels+1);
        //std::cout << "Refuels: " << burnup_target << std::endl;
    } else {
        return SS_enrich;
    }
    std::vector<cyclus::Material::Ptr> fissile_mani = cyclus::ResCast<cyclus::Material>(fissle.PopN(fissle.count()));
    std::vector<cyclus::Material::Ptr> non_fissile_mani = cyclus::ResCast<cyclus::Material>(non_fissle.PopN(non_fissle.count()));

    std::vector<std::vector<cyclus::Material::Ptr> > materials;
    for(int i = 0; i < inventory.size(); i++){
        std::vector<cyclus::Material::Ptr> manifest;
        manifest = cyclus::ResCast<cyclus::Material>(inventory[i].PopN(inventory[i].count()));
        materials.push_back(manifest);
    }

    double burnup_target = target_burnup;
    //finds total mass of this new batch
    double total_mass = core_mass / batches;
    //redefines target burnup to match batch
    cyclus::Material::Ptr mat = cyclus::Material::CreateUntracked(0, non_fissile_mani[0]->comp());
    int i = 0;
    std::map<std::string, double>::iterator it;
    for(it = incommods.begin(); it!=incommods.end(); ++it){
        double frac = it->second;
        if(materials[i][0]->quantity() > core_mass * frac){
            cyclus::Material::Ptr mat_temp = cyclus::Material::CreateUntracked(frac, materials[i][0]->comp());
            mat->Absorb(mat_temp);
            mass_frac -= frac;
        }
        i++;
    }
    // Starting blending of materials
    double fraction_1 = mass_frac;
    cyclus::Material::Ptr mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
    cyclus::Material::Ptr mat1 = cyclus::Material::CreateUntracked(mass_frac, fissile_mani[0]->comp());
    mat1->Absorb(mat_pass);
    //CompOut(mat1);
    //fuelBundle temp_bundle = comp_trans(mat1, fuel_library_);
    //double burnup_1 = burnupcalc_BU(temp_bundle, 2, 1, 40);
    fuelBundle temp_bundle = comp_function(mat1, fuel_library_);
    double burnup_1 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
    //Finding the second burnup iterator
    double fraction_2 = 0;
    mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
    mat1 = cyclus::Material::CreateUntracked(mass_frac, non_fissile_mani[0]->comp());
    mat1->Absorb(mat_pass);
    //CompOut(mat1);
    //temp_bundle = comp_trans(mat1, fuel_library_);
    //double burnup_2 = burnupcalc_BU(temp_bundle, 2, 1, 40);
    temp_bundle = comp_function(mat1, fuel_library_);
    double burnup_2 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
    //Finding the third burnup iterator
    /// TODO Reactor catch for extrapolation
    double fraction = (fraction_1) + (target_burnup - burnup_1)*((fraction_1 - fraction_2)/(burnup_1 - burnup_2));
    //std::cout <<  "fraction_1 "<<fraction_1 << " fraction_2 " << fraction_2 << " burnup_1 " << burnup_1 << " burnup_2 " << burnup_2 << std::endl;
    //std::cout << "fraction " << fraction << std::endl;
    mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
    mat1 = cyclus::Material::CreateUntracked(fraction, fissile_mani[0]->comp());
    mat1->Absorb(mat_pass);
    cyclus::Material::Ptr mat2 = cyclus::Material::CreateUntracked(mass_frac-fraction, non_fissile_mani[0]->comp());
    mat1->Absorb(mat2);
    //std::cout << "Fraction SU " << fraction << std::endl;
    //temp_bundle = comp_trans(mat1, fuel_library_);
    //double burnup_3 = burnupcalc_BU(temp_bundle, 2, 1, 40);
    temp_bundle = comp_function(mat1, fuel_library_);
    double burnup_3 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
    int inter = 0;
    //Using the iterators to calculate a Newton Method solution
    while(std::abs((target_burnup - burnup_3)/target_burnup) > 0.001){
        mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
        fraction_1 = fraction_2;
        fraction_2 = fraction;
        burnup_1 = burnup_2;
        burnup_2 = burnup_3;
        fraction = (fraction_1) + (target_burnup - burnup_1)*((fraction_1 - fraction_2)/(burnup_1 - burnup_2));
        //std::cout <<  "fraction_1 "<<fraction_1 << " fraction_2 " << fraction_2 << " burnup_1 " << burnup_1 << " burnup_2 " << burnup_2 << std::endl;
        //std::cout << "fraction " << fraction << std::endl;
        mat1 = cyclus::Material::CreateUntracked(fraction, fissile_mani[0]->comp());
        mat2 = cyclus::Material::CreateUntracked(mass_frac-fraction, non_fissile_mani[0]->comp());
        mat1->Absorb(mat2);
        mat1->Absorb(mat_pass);
        //temp_bundle = comp_trans(mat1, fuel_library_);
        //burnup_3 = burnupcalc_BU(temp_bundle, 2, 1, 40);
        temp_bundle = comp_function(mat1, fuel_library_);
        burnup_3 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
        if(inter == 50){
            continue;
        }
        inter++;
    }
    return_amount = fraction * total_mass * mass_frac;
    SS_enrich = return_amount;
    return return_amount;
}

double ReactorFacility::start_up(cyclus::toolkit::ResourceBuff fissle,
                                 cyclus::toolkit::ResourceBuff non_fissle,
                                 std::vector<cyclus::toolkit::ResourceBuff> inventory,
                                 std::map<std::string, double> incommods){
    double return_amount;
    double mass_frac = 1.;
    //Read the fuelfab inventory
    std::vector<cyclus::Material::Ptr> fissile_mani = cyclus::ResCast<cyclus::Material>(fissle.PopN(fissle.count()));
    std::vector<cyclus::Material::Ptr> non_fissile_mani = cyclus::ResCast<cyclus::Material>(non_fissle.PopN(non_fissle.count()));

    std::vector<std::vector<cyclus::Material::Ptr> > materials;
    for(int i = 0; i < inventory.size(); i++){
        std::vector<cyclus::Material::Ptr> manifest;
        manifest = cyclus::ResCast<cyclus::Material>(inventory[i].PopN(inventory[i].count()));
        materials.push_back(manifest);
    }
    double total_mass = core_mass / batches;
    cyclus::Material::Ptr mat = cyclus::Material::CreateUntracked(0, non_fissile_mani[0]->comp());
    int i = 0;
    std::map<std::string, double>::iterator it;
    for(it = incommods.begin(); it!=incommods.end(); ++it){
        double frac = it->second;
        if(materials[i][0]->quantity() > core_mass * frac){
            cyclus::Material::Ptr mat_temp = cyclus::Material::CreateUntracked(frac, materials[i][0]->comp());
            mat->Absorb(mat_temp);
            mass_frac -= frac;
        }
        i++;
    }
    /** TODO  - add in a precalculation of the total mass in the fuel from incommods
                this means check that there is enough in incommods and then create a
                new material for this section.
    **/
    // Starting blending of materials
    double fraction_1 = mass_frac;
    cyclus::Material::Ptr mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
    cyclus::Material::Ptr mat1 = cyclus::Material::CreateUntracked(mass_frac, fissile_mani[0]->comp());
    mat1->Absorb(mat_pass);
    fuelBundle temp_bundle = comp_function(mat1, fuel_library_);
    double burnup_1 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
    //Finding the second burnup iterator
    double fraction_2 = 0;
    mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
    mat1 = cyclus::Material::CreateUntracked(mass_frac, non_fissile_mani[0]->comp());
    mat1->Absorb(mat_pass);
    temp_bundle = comp_function(mat1, fuel_library_);
    double burnup_2 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
    //Finding the third burnup iterator
    /// TODO Reactor catch for extrapolation
    double fraction = (fraction_1) + (target_burnup - burnup_1)*((fraction_1 - fraction_2)/(burnup_1 - burnup_2));
    //std::cout <<  "fraction_1 "<<fraction_1 << " fraction_2 " << fraction_2 << " burnup_1 " << burnup_1 << " burnup_2 " << burnup_2 << std::endl;
    //std::cout << "fraction " << fraction << std::endl;
    mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
    mat1 = cyclus::Material::CreateUntracked(fraction, fissile_mani[0]->comp());
    mat1->Absorb(mat_pass);
    cyclus::Material::Ptr mat2 = cyclus::Material::CreateUntracked(mass_frac-fraction, non_fissile_mani[0]->comp());
    mat1->Absorb(mat2);
    //std::cout << "Fraction SU " << fraction << std::endl;
    temp_bundle = comp_function(mat1, fuel_library_);
    double burnup_3 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
    int inter = 0;
    //Using the iterators to calculate a Newton Method solution
    while(std::abs((target_burnup - burnup_3)/target_burnup) > 0.0001){
        mat_pass = cyclus::ResCast<cyclus::Material>(mat->Clone());
        fraction_1 = fraction_2;
        fraction_2 = fraction;
        burnup_1 = burnup_2;
        burnup_2 = burnup_3;
        fraction = (fraction_1) + (target_burnup - burnup_1)*((fraction_1 - fraction_2)/(burnup_1 - burnup_2));
        //std::cout <<  "fraction_1 "<<fraction_1 << " fraction_2 " << fraction_2 << " burnup_1 " << burnup_1 << " burnup_2 " << burnup_2 << std::endl;
        //std::cout << "fraction " << fraction << std::endl;
        mat1 = cyclus::Material::CreateUntracked(fraction, fissile_mani[0]->comp());
        mat2 = cyclus::Material::CreateUntracked(mass_frac-fraction, non_fissile_mani[0]->comp());
        mat1->Absorb(mat2);
        mat1->Absorb(mat_pass);
        temp_bundle = comp_function(mat1, fuel_library_);
        burnup_3 = SS_burnupcalc(temp_bundle.batch[0].collapsed_iso, batches, burnupcalc_timestep, nonleakage, fuel_library_.base_flux);
        if(inter == 50){
            continue;
        }
        inter++;
    }
    return_amount = fraction * total_mass * mass_frac;
    SS_enrich = return_amount;
    return return_amount;
}

void ReactorFacility::batch_reorder(){
//collapses each batch first, thgien orders them
    //std::cout << "Begin batch_reorder" << std::endl;
    double k0, k1;
    fuel_library_ = StructReader(fuel_library_);
    fuel_library_ = regionCollapse(fuel_library_);
    bool test = false;
    for(int i = 0; i < fuel_library_.batch.size(); i++){
        if(fuel_library_.batch[i].batch_fluence != 0){
            test = true;
        }
    }
    if(test == true){return;}

    fuelBundle temp_fuel = fuel_library_;
    fuel_library_.batch.clear();

    //goes through all the batches and orders them from lowest k to highest
    //uses temp_fuel to store the unordered batches
    while(temp_fuel.batch.size() != 0){
        int lowest = 0;
        for(int i = 1; i < temp_fuel.batch.size(); i++){
            k0 = temp_fuel.batch[lowest].collapsed_iso.neutron_prod[1]/temp_fuel.batch[lowest].collapsed_iso.neutron_dest[1];
            k1 = temp_fuel.batch[i].collapsed_iso.neutron_prod[1]/temp_fuel.batch[i].collapsed_iso.neutron_dest[1];
            if(k1 < k0){
                //if k of i'th batch is smaller than k of current lowest batch
                lowest = i;
            }
        }
        fuel_library_.batch.push_back(temp_fuel.batch[lowest]);
        temp_fuel.batch.erase(temp_fuel.batch.begin() + lowest);
    }
    //std::cout << " End batch_reorder" << std::endl;
}

///TODO Make this better.



void CompOutMat(cyclus::Material mat1){
    cyclus::CompMap comp;
    comp = mat1.comp()->mass(); //store the fractions of i'th batch in comp
    int comp_iso;
    cyclus::CompMap::iterator it;
    //each iso in comp
    for (it = comp.begin(); it != comp.end(); ++it){
        std::cout<<it->first<< " " << it->second << std::endl;
    }
}


extern "C" cyclus::Agent* ConstructReactorFacility(cyclus::Context* ctx) {
  return new ReactorFacility(ctx);
}

}
