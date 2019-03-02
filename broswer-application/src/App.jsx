import React from "react";
import { Col } from "reactstrap";
import { BrowserRouter, Route } from "react-router-dom";

import Home from "./pages/home/Home";
import Demo from "./pages/demo/GDPRDemo";
import Header from "./components/header/Header";
import Footer from "./components/footer/Footer";

const App = () => {
  return (
    <div id="gdpr-app">
      <BrowserRouter>
        <div id="app-route">
          <div className="container">
            <Header />
            <Route exact path="/" component={Home} />
            <Col
              xs={12}
              sm={{ size: 8, offset: 2 }}
              md={{ size: 8, offset: 2 }}
              lg={{ size: 6, offset: 3 }}
              className="route-content"
            >
              <Route path="/demo/:end" component={Demo} />
            </Col>
          </div>
          <Footer />
        </div>
      </BrowserRouter>
    </div>
  );
};

export default App;
