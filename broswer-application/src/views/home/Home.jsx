import React from "react";
import { Row, Col } from "reactstrap";
import { connect } from "react-redux";
import { withRouter } from "react-router-dom";

import Link from "../../components/links/Link";
import ECDHKeyChange from "../../components/ECDHKeyChange";

class Home extends React.Component {
  render() {
    return this.props.authenticated ? (
      <ECDHKeyChange />
    ) : (
        <Row className="text-center">
          <Col xs={12} className="base-margin-top base-margin-bottom">
            <Link name="personal" color="info" />
          </Col>
          <Col xs={12}>
            <Link name="business" color="warning" />
          </Col>
        </Row>
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
