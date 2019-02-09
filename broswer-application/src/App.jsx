import React from "react";
import { Col } from "reactstrap";
import { Provider } from "react-redux";
import createSagaMiddleware from "redux-saga";
import { createStore, applyMiddleware } from "redux";
import { BrowserRouter, Route } from "react-router-dom";
import { composeWithDevTools } from "redux-devtools-extension";

import saga from "./sagas";
import { reducers } from "./redux";

import Home from "./views/home/Home";
import Login from "./views/login/Login";
import Header from "./views/header/Header";
import Footer from "./components/footer/Footer";
import Register from "./views/register/Register";

import Demo from "./components/GDPRDemo";

const sagaMiddleware = createSagaMiddleware();

const store = createStore(
  reducers,
  composeWithDevTools(applyMiddleware(sagaMiddleware))
);

sagaMiddleware.run(saga);

const App = () => {
  return (
    <Provider store={store}>
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
              <Route path="/login" component={Login} />
              <Route path="/register" component={Register} />
            </Col>
          </div>
          <Footer />
        </div>
      </BrowserRouter>
    </Provider>
  );
};

export default App;
