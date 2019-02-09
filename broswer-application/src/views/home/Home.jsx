import React from "react";
import { Row, Col } from "reactstrap";
import { connect } from "react-redux";
import { withRouter } from "react-router-dom";

import Link from "../../components/links/Link";
import ECDHKeyChange from "../../components/ECDHKeyChange";

import PersonalUser from "../../static/images/personal-user.jpg";
import BusinessUser from "../../static/images/business-user.jpg";

class Home extends React.Component {
  render() {
    return this.props.authenticated ? (
      <ECDHKeyChange />
    ) : (
      <div id="app-home">
        <div id="banner">
          <Col
            xs={12}
            sm={{ size: 10, offset: 1 }}
            lg={{ size: 8, offset: 2 }}
            className="inner"
          >
            <Row>
              <h1>Welcome</h1>
            </Row>

            <Row className="dbl-margin-top">
              <Col xs={4}>
                <span class="fa fa-shield-alt" />
                <h3>Aliquam</h3>
                <p>Suspendisse amet ullamco</p>
              </Col>

              <Col xs={4} className="middle-block">
                <span class="fa fa-sms" />
                <h3>Elementum</h3>
                <p>Class aptent taciti ad litora</p>
              </Col>

              <Col xs={4}>
                <span class="fa fa-bug" />
                <h3>Ultrices</h3>
                <p>Nulla vitae mauris non felis</p>
              </Col>
            </Row>

            <Row className="dbl-margin-top">
              <a href="#user-entry" className="button">
                Get Started
              </a>
            </Row>
          </Col>
        </div>

        <Row id="user-entry" className="dbl-margin-top">
          <Col xs={12} md={6}>
            <Col
              xs={{ size: 8, offset: 2 }}
              md={{ size: 8, offset: 4 }}
              lg={{ size: 6, offset: 5 }}
            >
              <img
                className="img-round"
                src={PersonalUser}
                alt="personal user"
              />
              <h3>Personal User</h3>
              <p>
                Morbi in sem quis dui placerat ornare. Pellentesquenisi euismod
                in, pharetra a, ultricies in diam sed arcu. Cras consequat
                egestas augue vulputate.
              </p>
              <Link dest="personal" className="button" />
            </Col>
          </Col>

          <Col xs={12} md={6}>
            <Col
              xs={{ size: 8, offset: 2 }}
              md={{ size: 8 }}
              lg={{ size: 6, offset: 1 }}
            >
              <img
                className="img-round"
                src={BusinessUser}
                alt="business user"
              />
              <h3> Business User</h3>
              <p>
                Morbi in sem quis dui placerat ornare. Pellentesquenisi euismod
                in, pharetra a, ultricies in diam sed arcu. Cras consequat
                egestas augue vulputate.
              </p>
              <Link dest="business" className="button" />
            </Col>
          </Col>
        </Row>
      </div>
    );
  }
}

const mapStateToProps = state => ({
  authenticated: state.header.authentication.authenticated
});

const mapDispatchToProps = dispatch => ({
  dispatch: action => dispatch(action)
});

export default withRouter(
  connect(
    mapStateToProps,
    mapDispatchToProps
  )(Home)
);
