import tomviz.operators


class TestOperator(tomviz.operators.Operator):

    def transform_scalars(self, data):
        self.progress.update(100)
